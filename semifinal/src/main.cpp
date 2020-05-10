#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>    
#include <sys/stat.h>   
#include <sys/mman.h>
#include <fcntl.h>
#include <arm_neon.h>

#include <queue>
#include <iostream>
#include <algorithm>
#include <unordered_map>


class DirectedGraph {
public:
  DirectedGraph(const char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
      printf("file open error\n");
      exit(1);
    }

    struct stat st;
    int r = fstat(fd, &st);
    if (r == -1) {
      printf("get file stat error\n");
      close(fd);
      exit(-1);
    }
    int filesize = st.st_size;
    
    int8_t* p;
    int8_t* start; int8_t* current;
    int8_t  buf[16];

    p = (int8_t*)mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, 0);
    if (p == NULL || p == (void*)(-1)) {
      printf("mmap error\n");
      close(fd);
      exit(-1);
    }
    
    start = p; current = p; 
    int s;
    unsigned int* inputs = new unsigned int[4000000]; int inputs_size = 0;
    unsigned int* inputs_money = new unsigned int[2000000]; int inputs_money_size = 0;
    int split = 0;
    int8x16_t tmp;
    unsigned int max_id;
    unsigned int val1, val2;

    for (int i = 0; i < filesize; i++, current++) {
      if (*current == '\n' || *current == ',') {
        s = current - start;
        tmp = vld1q_s8(start);
        vst1q_s8(buf, tmp);
        buf[s] = '\0';

        switch(++split%3) {
          case 1: {
	          val1 = strtoul((char*)buf, NULL, 10);
	          start = current+1;
            break;
          }
          case 2: {
	          val2 = strtoul((char*)buf, NULL, 10);
	          start = current+1;
            inputs[inputs_size++] = val1;
            inputs[inputs_size++] = val2;
            max_id = val1 > max_id ? val1 : max_id;
            max_id = val2 > max_id ? val2 : max_id;
            break;
          }
          case 0: {
            inputs_money[inputs_money_size++] = strtoul((char*)buf, NULL, 10);
	          start = current+1;
            break;
          }
        }
      }
    }

    munmap((void*)p, filesize);
    close(fd);

    unsigned int* ids = new unsigned int[inputs_size];
    memcpy(ids, inputs, sizeof(unsigned int)*inputs_size);
    std::sort(ids, ids+inputs_size);
    ids_num_ = std::unique(ids, ids+inputs_size) - ids;

    std::unordered_map<unsigned int, int> m;
    ids_comma_ = new int8_t[ids_num_*16];
    ids_line_  = new int8_t[ids_num_*16];
    sl_= new int[ids_num_];

    unsigned int id, t;
    for (int i = 0; i < ids_num_; i++) {
      id = ids[i];
      m[id] = i;
      t = sprintf((char*)ids_comma_+i*16, "%u,",  id);
      sprintf((char*)ids_line_+i*16, "%u\n", id);
      sl_[i] = t;
    }

    delete[](ids);

    G_     = std::vector<std::vector<int>>(ids_num_); 
    inv_G_ = std::vector<std::vector<int>>(ids_num_); 

    int send_idx, recv_idx;
    unsigned int money;
    for (int i = 0, j = 0; i < inputs_size; i+=2, j++) {
      send_idx = m[inputs[i]]; recv_idx = m[inputs[i+1]]; money = inputs_money[j];
      G_[send_idx].emplace_back(recv_idx);
      inv_G_[recv_idx].emplace_back(send_idx);
      money_hash_[(unsigned long long)send_idx << 32 | recv_idx] = money;
    }

    delete[](inputs); delete[](inputs_money);

    for (int i = 0; i < ids_num_; i++)
      std::sort(G_[i].begin(), G_[i].end());
  }

  ~DirectedGraph() {
    delete[](ret3_); delete[](ret4_); delete[](ret5_); delete[](ret6_); delete[](ret7_);
    delete[](status_map_);
    delete[](ids_comma_); delete[](ids_line_); delete[](sl_);
  }

  inline bool IsGoodProportion(int idx1, int idx2, int idx3) {
    if (money_hash_[(unsigned long long)idx1 << 32 | idx2] >= money_hash_[(unsigned long long)idx2 << 32 | idx3] / 3.0 && 
        money_hash_[(unsigned long long)idx2 << 32 | idx3] >= 0.2 * money_hash_[(unsigned long long)idx1 << 32 | idx2])
    return true;
    else return false;
  }

  void FindAllCycles() {
    status_map_ = new short[ids_num_]();

    int idx1, idx2, idx3, idx4, idx5, idx6, idx7;

    std::vector<int> effective_idxes;
    effective_idxes.reserve(100000);

    int8_t* s3 = ret_[0];
    int8_t* s4 = ret_[1];
    int8_t* s5 = ret_[2];
    int8_t* s6 = ret_[3];
    int8_t* s7 = ret_[4];
    int s;

    int8x16_t tmp;

    for (idx1 = 0; idx1 < ids_num_; idx1++) {
      if (G_[idx1].empty()) continue;

      for (auto& idx2 : inv_G_[idx1]) {
        if (idx2 < idx1) continue;
        status_map_[idx2] = 3;
	      effective_idxes.emplace_back(idx2);

        for (auto& idx3 : inv_G_[idx2]) {
          if (idx3 <= idx1) continue;
          status_map_[idx3] = status_map_[idx3] < 2 ? 2 : status_map_[idx3];
          effective_idxes.emplace_back(idx3);

          for (auto& idx4 : inv_G_[idx3]) {
            if (idx4 <= idx1) continue;
            status_map_[idx4] = status_map_[idx4] == 0 ? 1 : status_map_[idx4];
            effective_idxes.emplace_back(idx4);
          }
        }
      }

      for (auto& idx2 : G_[idx1]) {
        if (idx2 < idx1 || G_[idx2].empty()) continue;

        status_map_[idx2] = status_map_[idx2] == 0 ? -4 : (-1)*status_map_[idx2];
        for (auto& idx3 : G_[idx2]) {
          if (idx3 <= idx1) continue;
          if (!IsGoodProportion(idx1,idx2,idx3)) continue;

	        if (status_map_[idx3] == 3 && IsGoodProportion(idx2,idx3,idx1) && IsGoodProportion(idx3,idx1,idx2)) {
            tmp = vld1q_s8(ids_comma_+idx1*16); vst1q_s8(s3, tmp); s = sl_[idx1]; ret_num_[0] += s; s3 += s;
            tmp = vld1q_s8(ids_comma_+idx2*16); vst1q_s8(s3, tmp); s = sl_[idx2]; ret_num_[0] += s; s3 += s;
            tmp = vld1q_s8(ids_line_+idx3*16);  vst1q_s8(s3, tmp); s = sl_[idx3]; ret_num_[0] += s; s3 += s;
	          path_num_++;
	        }
          
          status_map_[idx3] = status_map_[idx3] == 0 ? -4 : (-1)*status_map_[idx3];
          for (auto& idx4 : G_[idx3]) {
            if (idx4 > idx1 && status_map_[idx4] >= 0 && IsGoodProportion(idx2,idx3,idx4)) {
	            if (status_map_[idx4] == 3 && IsGoodProportion(idx3,idx4,idx1) && IsGoodProportion(idx4,idx1,idx2)) {
                tmp = vld1q_s8(ids_comma_+idx1*16); vst1q_s8(s4, tmp); s = sl_[idx1]; ret_num_[1] += s; s4 += s;
                tmp = vld1q_s8(ids_comma_+idx2*16); vst1q_s8(s4, tmp); s = sl_[idx2]; ret_num_[1] += s; s4 += s;
                tmp = vld1q_s8(ids_comma_+idx3*16); vst1q_s8(s4, tmp); s = sl_[idx3]; ret_num_[1] += s; s4 += s;
                tmp = vld1q_s8(ids_line_+idx4*16);  vst1q_s8(s4, tmp); s = sl_[idx4]; ret_num_[1] += s; s4 += s;
	              path_num_++;
	            }

              status_map_[idx4] = status_map_[idx4] == 0 ? -4 : (-1)*status_map_[idx4];
              for (auto& idx5 : G_[idx4]) {
                if (status_map_[idx5] > 0 && IsGoodProportion(idx3,idx4,idx5)) {
	                if (status_map_[idx5] == 3 && IsGoodProportion(idx4,idx5,idx1) && IsGoodProportion(idx5,idx1,idx2)) {
                    tmp = vld1q_s8(ids_comma_+idx1*16); vst1q_s8(s5, tmp); s = sl_[idx1]; ret_num_[2] += s; s5 += s;
                    tmp = vld1q_s8(ids_comma_+idx2*16); vst1q_s8(s5, tmp); s = sl_[idx2]; ret_num_[2] += s; s5 += s;
                    tmp = vld1q_s8(ids_comma_+idx3*16); vst1q_s8(s5, tmp); s = sl_[idx3]; ret_num_[2] += s; s5 += s;
                    tmp = vld1q_s8(ids_comma_+idx4*16); vst1q_s8(s5, tmp); s = sl_[idx4]; ret_num_[2] += s; s5 += s;
                    tmp = vld1q_s8(ids_line_+idx5*16);  vst1q_s8(s5, tmp); s = sl_[idx5]; ret_num_[2] += s; s5 += s;
	                  path_num_++;
	                }

                  status_map_[idx5] *= -1;
                  for (auto& idx6 : G_[idx5]) {
		                if (status_map_[idx6] > 1 && IsGoodProportion(idx4,idx5,idx6)) {
	                    if (status_map_[idx6] == 3 && IsGoodProportion(idx5,idx6,idx1) && IsGoodProportion(idx6,idx1,idx2)) {
                        tmp = vld1q_s8(ids_comma_+idx1*16); vst1q_s8(s6, tmp); s = sl_[idx1]; ret_num_[3] += s; s6 += s;
                        tmp = vld1q_s8(ids_comma_+idx2*16); vst1q_s8(s6, tmp); s = sl_[idx2]; ret_num_[3] += s; s6 += s;
                        tmp = vld1q_s8(ids_comma_+idx3*16); vst1q_s8(s6, tmp); s = sl_[idx3]; ret_num_[3] += s; s6 += s;
                        tmp = vld1q_s8(ids_comma_+idx4*16); vst1q_s8(s6, tmp); s = sl_[idx4]; ret_num_[3] += s; s6 += s;
                        tmp = vld1q_s8(ids_comma_+idx5*16); vst1q_s8(s6, tmp); s = sl_[idx5]; ret_num_[3] += s; s6 += s;
                        tmp = vld1q_s8(ids_line_+idx6*16);  vst1q_s8(s6, tmp); s = sl_[idx6]; ret_num_[3] += s; s6 += s;
	                      path_num_++;
	                    }

                      for (auto& idx7 : G_[idx6]) {
			                  if (status_map_[idx7] == 3 && IsGoodProportion(idx5,idx6,idx7) && IsGoodProportion(idx6,idx7,idx1) && IsGoodProportion(idx7,idx1,idx2)) {
                          tmp = vld1q_s8(ids_comma_+idx1*16); vst1q_s8(s7, tmp); s = sl_[idx1]; ret_num_[4] += s; s7 += s;
                          tmp = vld1q_s8(ids_comma_+idx2*16); vst1q_s8(s7, tmp); s = sl_[idx2]; ret_num_[4] += s; s7 += s;
                          tmp = vld1q_s8(ids_comma_+idx3*16); vst1q_s8(s7, tmp); s = sl_[idx3]; ret_num_[4] += s; s7 += s;
                          tmp = vld1q_s8(ids_comma_+idx4*16); vst1q_s8(s7, tmp); s = sl_[idx4]; ret_num_[4] += s; s7 += s;
                          tmp = vld1q_s8(ids_comma_+idx5*16); vst1q_s8(s7, tmp); s = sl_[idx5]; ret_num_[4] += s; s7 += s;
                          tmp = vld1q_s8(ids_comma_+idx6*16); vst1q_s8(s7, tmp); s = sl_[idx6]; ret_num_[4] += s; s7 += s;
                          tmp = vld1q_s8(ids_line_+idx7*16);  vst1q_s8(s7, tmp); s = sl_[idx7]; ret_num_[4] += s; s7 += s;
	                        path_num_++;
			                  }
		                  }    
		                }
                  }
                  status_map_[idx5] *= -1;
                }
              }
              status_map_[idx4] = status_map_[idx4] == -4 ? 0 : (-1)*status_map_[idx4];
            }
          }
          status_map_[idx3] = status_map_[idx3] == -4 ? 0 : (-1)*status_map_[idx3];
        }
        status_map_[idx2] = status_map_[idx2] == -4 ? 0 : (-1)*status_map_[idx2];
      }

      for (int& effective_idx : effective_idxes) {
        status_map_[effective_idx] = 0;
      }
      effective_idxes.clear();
    }
  }

  void WriteFile(const char* filename) {
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
      printf("file open error\n");
      exit(1);
    }
    
    char buf[15];
    sprintf(buf, "%d\n", path_num_);
    fwrite(buf, strlen(buf), sizeof(char), fp);

    for (int i = 0; i < 5; i++)
      fwrite((char*)ret_[i], ret_num_[i], sizeof(char), fp);

    fclose(fp);
  }

private:
  int8_t* ids_comma_;
  int8_t* ids_line_;
  int* sl_;

  int ids_num_ = 0;

  std::vector<std::vector<int>> G_;
  std::vector<std::vector<int>> inv_G_;
  short* status_map_;
  std::unordered_map<unsigned long long, unsigned int> money_hash_;

  int8_t* ret3_ = new int8_t[33*20000000]; 
  int8_t* ret4_ = new int8_t[44*20000000]; 
  int8_t* ret5_ = new int8_t[55*20000000];
  int8_t* ret6_ = new int8_t[66*20000000];
  int8_t* ret7_ = new int8_t[77*20000000];
  int8_t* ret_[5] = {ret3_, ret4_, ret5_, ret6_, ret7_};
  int ret_num_[5] = {0, 0, 0, 0, 0}; 
  int path_num_ = 0; 
};


int main(int argc, char** argv) {
  // DirectedGraph directed_graph("../data/test_data.txt");
  // DirectedGraph directed_graph("/root/dataset/28W/test_data28W.txt");
  // DirectedGraph directed_graph("/root/dataset/200W/test_data_N111314_E200W_A19630345.txt");
  DirectedGraph directed_graph("/data/test_data.txt");

  directed_graph.FindAllCycles();

  // directed_graph.WriteFile("result.txt");
  directed_graph.WriteFile("/projects/student/result.txt");

  return 0;
}
