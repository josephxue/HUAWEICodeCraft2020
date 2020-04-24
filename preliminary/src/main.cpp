#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>    
#include <sys/stat.h>   
#include <sys/mman.h>
#include <fcntl.h>
#include <arm_neon.h>

#include <queue>
#include <algorithm>


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
    int* inputs = new int[560000]; int inputs_size = 0;
    int split = 0;
    int8x16_t tmp;
    int max_id = -1;
    int val;
    for (int i = 0; i < filesize; i++, current++) {
      if (*current == '\n' || *current == ',') {
	      if (++split % 3 != 0) {
          s = current - start;
	        tmp = vld1q_s8(start);
	        vst1q_s8(buf, tmp);
	        buf[s] = '\0';
	        val = atoi((char*)buf);
          inputs[inputs_size++] = val;
	        max_id = val > max_id ? val : max_id;
	      }
	      start = current+1;
      }
    }

    munmap((void*)p, filesize);
    close(fd);
    
    int* m = new int[max_id+1]; 
    int* c = new int[max_id+1]();
    int* ids = new int[max_id+1];
    for (int i = 0; i < inputs_size; i++)
      c[inputs[i]]++;
    for (int i = 0; i < max_id+1; i++) {
      if (c[i] > 0) {
        ids[ids_num_] = i;
        m[i] = ids_num_++;
      }
    }

    delete[](c);

    ids_comma_ = new char[ids_num_*16];
    ids_line_  = new char[ids_num_*16];
    sl_= new int [ids_num_];

    int id, t;
    for (int i = 0; i < ids_num_; i++) {
      id = ids[i];
      t = sprintf(ids_comma_+i*16, "%d,",  id);
      sprintf(ids_line_+i*16, "%d\n", id);
      sl_[i] = t;
    }

    delete[](ids);

    G_     = new int[ids_num_*50];
    inv_G_ = new int[ids_num_*50];
    in_degrees_  = new int[ids_num_]();
    out_degrees_ = new int[ids_num_]();

    int send_idx = -1; int recv_idx = -1;
    for (int i = 0; i < inputs_size; i+=2) {
      send_idx = m[inputs[i]]; recv_idx = m[inputs[i+1]];
      G_[send_idx*50+out_degrees_[send_idx]] = recv_idx;
      inv_G_[recv_idx*50+in_degrees_[recv_idx]] = send_idx;
      in_degrees_[recv_idx]++;
      out_degrees_[send_idx]++;
    }

    delete[](inputs); delete[](m);

    // topo sort
    std::queue<int> q;
    int u, v;

    for (int i = 0; i < ids_num_; i++)
      if (0 == in_degrees_[i]) q.push(i);
    while (!q.empty()) {
      u = q.front();
      q.pop();
      for (int i = 0; i < out_degrees_[u]; i++) {
        v = G_[u*50+i];
	      for (int j = 0; j < in_degrees_[v]; j++) {
	        if (inv_G_[v*50+j] == u) inv_G_[v*50+j] = inv_G_[v*50+in_degrees_[v]-1];
	      }
        if (0 == --in_degrees_[v]) q.push(v);
      }
    }

    for (int i = 0; i < ids_num_; i++)
      if (0 == out_degrees_[i]) q.push(i);
    while (!q.empty()) {
      u = q.front();
      q.pop();
      for (int i = 0; i < in_degrees_[u]; i++) {
        v = inv_G_[u*50+i];
	      for (int j = 0; j < out_degrees_[v]; j++) {
	        if (G_[v*50+j] == u) G_[v*50+j] = G_[v*50+out_degrees_[v]-1];
	      }
        if (0 == --out_degrees_[v]) q.push(v);
      }
    }

    bool is_sorted = false;
    int tmpsort;
    for (int i = 0; i < ids_num_; i++) {
      if (in_degrees_[i] == 0) out_degrees_[i] = 0;
      if (out_degrees_[i] > 1) {
        for (int j = 0; j < out_degrees_[i]-1; j++) {
          is_sorted = true;
          for (int k = 0; k < out_degrees_[i]-1-j; k++) {
            if (*(G_+i*50+k) > *(G_+i*50+k+1)) {
              is_sorted = false;
              tmpsort = *(G_+i*50+k);
              *(G_+i*50+k) = *(G_+i*50+k+1);
              *(G_+i*50+k+1) = tmpsort;
            }
          }
          if(is_sorted) break;
        }
      }
    }
  }

  ~DirectedGraph() {
    delete[](ret3_); delete[](ret4_); delete[](ret5_); delete[](ret6_); delete[](ret7_);
    delete[](in_degrees_); delete[](out_degrees_);
    delete[](G_);
    delete[](inv_G_);
    delete[](status_map_);
    delete[](ids_comma_); delete[](ids_line_); delete[](sl_);
  }

  void FindAllCycles() {
    status_map_ = new bool[ids_num_*3]();

    int idx1, idx2, idx3, idx4, idx5, idx6, idx7;

    int tail_idxes[50]; int tail_idxes_size = 0;
    int* effective_idxes = new int[125000]; int effective_idxes_size = 0;

    char* s3 = ret_[0];
    char* s4 = ret_[1];
    char* s5 = ret_[2];
    char* s6 = ret_[3];
    char* s7 = ret_[4];
    int s;

    int8x16_t tmp;

    for (idx1 = 0; idx1 < ids_num_; idx1++) {
      if (out_degrees_[idx1] == 0) continue;
      if (G_[idx1*50+out_degrees_[idx1]-1] < idx1) continue;

      for (int i = 0; i < in_degrees_[idx1]; i++) {
        idx2 = inv_G_[idx1*50+i];
        if (idx2 <= idx1) continue;
        status_map_[idx2*3+1] = true; status_map_[idx2*3+2] = true;
	      effective_idxes[effective_idxes_size++] = idx2;
        tail_idxes[tail_idxes_size++] = idx2;

        for (int j = 0; j < in_degrees_[idx2]; j++) {
          idx3 = inv_G_[idx2*50+j];
          if (idx3 <= idx1) continue;
          status_map_[idx3*3+1] = true; effective_idxes[effective_idxes_size++] = idx3;

          for (int k = 0; k < in_degrees_[idx3]; k++) {
            idx4 = inv_G_[idx3*50+k];
            if (idx4 <= idx1) continue;
            status_map_[idx4*3+1] = true; effective_idxes[effective_idxes_size++] = idx4;
          }
        }
      }

      for (int i = 0; i < out_degrees_[idx1]; i++) {
        idx2 = G_[idx1*50+i];
        if (idx2 < idx1 || out_degrees_[idx2] == 0) continue;
        if (G_[idx2*50+out_degrees_[idx2]-1] < idx1) continue;
        status_map_[idx2*3] = true;

        for (int j = 0; j < out_degrees_[idx2]; j++) {
          idx3 = G_[idx2*50+j];
          if (idx3 <= idx1) continue;
          status_map_[idx3*3] = true;

	        if (status_map_[idx3*3+2] == true) {
            tmp = vld1q_s8((int8_t*)(ids_comma_+idx1*16)); vst1q_s8((int8_t*)s3, tmp); s = sl_[idx1]; ret_num_[0] += s; s3 += s;
            tmp = vld1q_s8((int8_t*)(ids_comma_+idx2*16)); vst1q_s8((int8_t*)s3, tmp); s = sl_[idx2]; ret_num_[0] += s; s3 += s;
            tmp = vld1q_s8((int8_t*)(ids_line_+idx3*16));  vst1q_s8((int8_t*)s3, tmp); s = sl_[idx3]; ret_num_[0] += s; s3 += s;
	          path_num_++;
	        }

          for (int k = 0; k < out_degrees_[idx3]; k++) {
            idx4 = G_[idx3*50+k];

            if (idx4 > idx1 && status_map_[idx4*3] == false) {
              status_map_[idx4*3] = true;

	            if (status_map_[idx4*3+2] == true) {
                tmp = vld1q_s8((int8_t*)(ids_comma_+idx1*16)); vst1q_s8((int8_t*)s4, tmp); s = sl_[idx1]; ret_num_[1] += s; s4 += s;
                tmp = vld1q_s8((int8_t*)(ids_comma_+idx2*16)); vst1q_s8((int8_t*)s4, tmp); s = sl_[idx2]; ret_num_[1] += s; s4 += s;
                tmp = vld1q_s8((int8_t*)(ids_comma_+idx3*16)); vst1q_s8((int8_t*)s4, tmp); s = sl_[idx3]; ret_num_[1] += s; s4 += s;
                tmp = vld1q_s8((int8_t*)(ids_line_+idx4*16));  vst1q_s8((int8_t*)s4, tmp); s = sl_[idx4]; ret_num_[1] += s; s4 += s;
	              path_num_++;
	            }

              for (int l = 0; l < out_degrees_[idx4]; l++) {
                idx5 = G_[idx4*50+l];

                if (status_map_[idx5*3] == false && status_map_[idx5*3+1] == true) {
                  status_map_[idx5*3] = true;

	                if (status_map_[idx5*3+2] == true) {
                    tmp = vld1q_s8((int8_t*)(ids_comma_+idx1*16)); vst1q_s8((int8_t*)s5, tmp); s = sl_[idx1]; ret_num_[2] += s; s5 += s;
                    tmp = vld1q_s8((int8_t*)(ids_comma_+idx2*16)); vst1q_s8((int8_t*)s5, tmp); s = sl_[idx2]; ret_num_[2] += s; s5 += s;
                    tmp = vld1q_s8((int8_t*)(ids_comma_+idx3*16)); vst1q_s8((int8_t*)s5, tmp); s = sl_[idx3]; ret_num_[2] += s; s5 += s;
                    tmp = vld1q_s8((int8_t*)(ids_comma_+idx4*16)); vst1q_s8((int8_t*)s5, tmp); s = sl_[idx4]; ret_num_[2] += s; s5 += s;
                    tmp = vld1q_s8((int8_t*)(ids_line_+idx5*16));  vst1q_s8((int8_t*)s5, tmp); s = sl_[idx5]; ret_num_[2] += s; s5 += s;
	                  path_num_++;
	                }

                  for (int m = 0; m < out_degrees_[idx5]; m++) {
                    idx6 = G_[idx5*50+m];

		                if (status_map_[idx6*3] == false && status_map_[idx6*3+1] == true) {
                      status_map_[idx6*3] = true;

	                    if (status_map_[idx6*3+2] == true) {
                        tmp = vld1q_s8((int8_t*)(ids_comma_+idx1*16)); vst1q_s8((int8_t*)s6, tmp); s = sl_[idx1]; ret_num_[3] += s; s6 += s;
                        tmp = vld1q_s8((int8_t*)(ids_comma_+idx2*16)); vst1q_s8((int8_t*)s6, tmp); s = sl_[idx2]; ret_num_[3] += s; s6 += s;
                        tmp = vld1q_s8((int8_t*)(ids_comma_+idx3*16)); vst1q_s8((int8_t*)s6, tmp); s = sl_[idx3]; ret_num_[3] += s; s6 += s;
                        tmp = vld1q_s8((int8_t*)(ids_comma_+idx4*16)); vst1q_s8((int8_t*)s6, tmp); s = sl_[idx4]; ret_num_[3] += s; s6 += s;
                        tmp = vld1q_s8((int8_t*)(ids_comma_+idx5*16)); vst1q_s8((int8_t*)s6, tmp); s = sl_[idx5]; ret_num_[3] += s; s6 += s;
                        tmp = vld1q_s8((int8_t*)(ids_line_+idx6*16));  vst1q_s8((int8_t*)s6, tmp); s = sl_[idx6]; ret_num_[3] += s; s6 += s;
	                      path_num_++;
	                    }

                      for (int n = 0; n < out_degrees_[idx6]; n++) {
                        idx7 = G_[idx6*50+n];

			                  if (status_map_[idx7*3] == false && status_map_[idx7*3+1] == true && status_map_[idx7*3+2] == true) {
                          tmp = vld1q_s8((int8_t*)(ids_comma_+idx1*16)); vst1q_s8((int8_t*)s7, tmp); s = sl_[idx1]; ret_num_[4] += s; s7 += s;
                          tmp = vld1q_s8((int8_t*)(ids_comma_+idx2*16)); vst1q_s8((int8_t*)s7, tmp); s = sl_[idx2]; ret_num_[4] += s; s7 += s;
                          tmp = vld1q_s8((int8_t*)(ids_comma_+idx3*16)); vst1q_s8((int8_t*)s7, tmp); s = sl_[idx3]; ret_num_[4] += s; s7 += s;
                          tmp = vld1q_s8((int8_t*)(ids_comma_+idx4*16)); vst1q_s8((int8_t*)s7, tmp); s = sl_[idx4]; ret_num_[4] += s; s7 += s;
                          tmp = vld1q_s8((int8_t*)(ids_comma_+idx5*16)); vst1q_s8((int8_t*)s7, tmp); s = sl_[idx5]; ret_num_[4] += s; s7 += s;
                          tmp = vld1q_s8((int8_t*)(ids_comma_+idx6*16)); vst1q_s8((int8_t*)s7, tmp); s = sl_[idx6]; ret_num_[4] += s; s7 += s;
                          tmp = vld1q_s8((int8_t*)(ids_line_+idx7*16));  vst1q_s8((int8_t*)s7, tmp); s = sl_[idx7]; ret_num_[4] += s; s7 += s;
	                        path_num_++;
			                  }
		                  }    
                      status_map_[idx6*3] = false;
		                }
                  }
                  status_map_[idx5*3] = false;
                }
              }
              status_map_[idx4*3] = false;
            }
          }
          status_map_[idx3*3] = false;
        }
        status_map_[idx2*3] = false;
      }

      for (int i = 0; i < tail_idxes_size; i++) {
        status_map_[tail_idxes[i]*3+2] = false;
      }
      for (int i = 0; i < effective_idxes_size; i++) {
        status_map_[effective_idxes[i]*3+1] = false;
      }
      tail_idxes_size = 0; effective_idxes_size = 0;
    }
    delete[](effective_idxes);
  }

  void WriteFile(const char* filename) {
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
      printf("file open error\n");
      exit(1);
    }
    
    int bias = 0;
    char buf[15];
    const int block_size = 1024*1024;
    
    sprintf(buf, "%d\n", path_num_);
    fwrite(buf, strlen(buf), sizeof(char), fp);

    for (int i = 0; i < 5; i++) {
      while (ret_num_[i] >= block_size) {
        fwrite(ret_[i]+bias, block_size, sizeof(char), fp);
        ret_num_[i] -= block_size;
        bias += block_size;
      }
      fwrite(ret_[i]+bias, ret_num_[i], sizeof(char), fp);
      bias = 0;
    }
    fclose(fp);
  }

private:
  char* ids_comma_;
  char* ids_line_;
  int* sl_;

  int ids_num_ = 0;

  int* G_;
  int* inv_G_;
  bool* status_map_;
  int* in_degrees_;
  int* out_degrees_;
  char* ret3_ = new char[33*500000+10]; 
  char* ret4_ = new char[44*500000+10]; 
  char* ret5_ = new char[55*1000000+10];
  char* ret6_ = new char[66*2000000+10];
  char* ret7_ = new char[77*3000000+10];
  char* ret_[5] = {ret3_, ret4_, ret5_, ret6_, ret7_};
  int ret_num_[5] = {0, 0, 0, 0, 0}; 
  int ret_step_[5] = {4, 4, 8, 8, 8};
  int path_num_ = 0; 
};


int main(int argc, char** argv) {
  // DirectedGraph directed_graph("../data/test_data.txt");
  // DirectedGraph directed_graph("/root/2020HuaweiCodecraft-TestData/1004812/test_data.txt");
  DirectedGraph directed_graph("/data/test_data.txt");

  directed_graph.FindAllCycles();

  // directed_graph.WriteFile("go.txt");
  directed_graph.WriteFile("/projects/student/result.txt");

  return 0;
}
