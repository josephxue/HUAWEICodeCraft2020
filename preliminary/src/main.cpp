#include <stdio.h>
#include <string.h>
#include <arm_neon.h>

#include <queue>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>


class DirectedGraph {
public:
  DirectedGraph(std::string filename) {
    FILE *fp = fopen(filename.c_str(), "r");
    if (fp == NULL) {
      printf("file open error\n");
      exit(1);
    }

    int* inputs = new int[560000]; int inputs_size = 0;
    int send_id, recv_id, val;
    while (fscanf(fp, "%d,%d,%d", &send_id, &recv_id, &val) > 0) {
      inputs[inputs_size]   = send_id;
      inputs[inputs_size+1] = recv_id;
      inputs_size += 2;
    }
    fclose(fp);

    int* ids = new int[inputs_size];
    memcpy(ids, inputs, inputs_size*sizeof(int));
    std::sort(ids, ids+inputs_size);
    ids_num_ = std::unique(ids, ids+inputs_size) - ids;

    std::unordered_map<int, int> m;

    ids_comma_ = std::vector<std::string>(ids_num_);
    ids_line_  = std::vector<std::string>(ids_num_);

    int id;
    for (int i = 0; i < ids_num_; i++) {
      id = ids[i];
      ids_comma_[i] = std::to_string(id) + ',';
      ids_line_[i]  = std::to_string(id) + '\n';
      m[id] = i;
    }
    delete[](ids);

    G_     = new int[280000*50];
    inv_G_ = new int[280000*50];
    in_degrees_  = new int[ids_num_]; memset(in_degrees_,  0, ids_num_*sizeof(int));
    out_degrees_ = new int[ids_num_]; memset(out_degrees_, 0, ids_num_*sizeof(int));

    int send_idx = -1; int recv_idx = -1;
    for (int i = 0; i < inputs_size; i+=2) {
      send_idx = m[inputs[i]]; recv_idx = m[inputs[i+1]];
      G_[send_idx*50+out_degrees_[send_idx]] = recv_idx;
      inv_G_[recv_idx*50+in_degrees_[recv_idx]] = send_idx;
      in_degrees_[recv_idx]++;
      out_degrees_[send_idx]++;
    }

    delete[](inputs);

    // topo sort
    int* tmp1 = new int[ids_num_]; memcpy(tmp1,  in_degrees_, ids_num_*sizeof(int));
    int* tmp2 = new int[ids_num_]; memcpy(tmp2, out_degrees_, ids_num_*sizeof(int));

    std::queue<int> q;
    int u, v;

    for (int i = 0; i < ids_num_; i++)
      if (0 == tmp1[i]) q.push(i);
    while (!q.empty()) {
      u = q.front();
      q.pop();
      for (int i = 0; i < out_degrees_[u]; i++) {
        v = G_[u*50+i];
        if (0 == --tmp1[v]) q.push(v);
      }
    }
    for (int i = 0; i < ids_num_; i++) {
      if (tmp1[i] == 0) out_degrees_[i] = 0;
    }

    for (int i = 0; i < ids_num_; i++)
      if (0 == tmp2[i]) q.push(i);
    while (!q.empty()) {
      u = q.front();
      q.pop();
      for (int i = 0; i < in_degrees_[u]; i++) {
        v = inv_G_[u*50+i];
        if (0 == --tmp2[v]) q.push(v);
      }
    }
    for (int i = 0; i < ids_num_; i++) {
      if (tmp2[i] == 0) out_degrees_[i] = 0;
      std::sort(G_+i*50, G_+i*50+out_degrees_[i]);
    }

    delete[](tmp1); delete[](tmp2);
  }

  ~DirectedGraph() {
    delete[](ret3_); delete[](ret4_); delete[](ret5_); delete[](ret6_); delete[](ret7_);
    delete[](in_degrees_); delete[](out_degrees_);
    delete[](G_);
    delete[](inv_G_);
    delete[](status_map_);
  }

  void FindAllCycles() {
    int32x4_t p;

    status_map_ = new bool[ids_num_*3]; memset(status_map_, false, ids_num_*3*sizeof(bool));

    int idx1, idx2, idx3, idx4, idx5, idx6, idx7;

    int* tail_idxes = new int[50]; int tail_idxes_size = 0;
    int* effective_idxes = new int[125000]; int effective_idxes_size = 0;

    int bias;
    int path[8];
    for (idx1 = 0; idx1 < ids_num_; idx1++) {
      if (out_degrees_[idx1] == 0) continue;
      if (G_[idx1*50+out_degrees_[idx1]-1] < idx1) continue;
      path[0] = idx1;

      status_map_[idx1*3+1] = true; effective_idxes[effective_idxes_size] = idx1; effective_idxes_size++;
      for (int i = 0; i < in_degrees_[idx1]; i++) {
        idx2 = inv_G_[idx1*50+i];
        if (idx2 < idx1) continue;
        status_map_[idx2*3+1] = true; effective_idxes[effective_idxes_size] = idx2; effective_idxes_size++;
        status_map_[idx2*3+2] = true; tail_idxes[tail_idxes_size] = idx2; tail_idxes_size++; 

        for (int j = 0; j < in_degrees_[idx2]; j++) {
          idx3 = inv_G_[idx2*50+j];
          if (idx3 < idx1) continue;
          status_map_[idx3*3+1] = true; effective_idxes[effective_idxes_size] = idx3; effective_idxes_size++;

          for (int k = 0; k < in_degrees_[idx3]; k++) {
            idx4 = inv_G_[idx3*50+k];
            if (idx4 < idx1) continue;
            status_map_[idx4*3+1] = true; effective_idxes[effective_idxes_size] = idx4; effective_idxes_size++;
          }
        }
      }

      for (int i = 0; i < out_degrees_[idx1]; i++) {
        idx2 = G_[idx1*50+i];
        if (idx2 < idx1 || out_degrees_[idx2] == 0) continue;
        if (G_[idx2*50+out_degrees_[idx2]-1] < idx1) continue;
        path[1] = idx2;
        status_map_[idx2*3] = true;

        for (int j = 0; j < out_degrees_[idx2]; j++) {
          idx3 = G_[idx2*50+j];
          if (idx3 <= idx1) continue;
          path[2] = idx3;
          status_map_[idx3*3] = true;

          for (int k = 0; k < out_degrees_[idx3]; k++) {
            idx4 = G_[idx3*50+k];
            if (idx4 < idx1) continue;
            if (idx4 == idx1) {
              p = vld1q_s32(path);
              vst1q_s32(ret_[0]+ret_num_[0]*4, p);
              ret_num_[0]++;
              continue;
            }
            if (status_map_[idx4*3] == false) {
              path[3] = idx4;
              status_map_[idx4*3] = true;

              for (int l = 0; l < out_degrees_[idx4]; l++) {
                idx5 = G_[idx4*50+l];
                if (status_map_[idx5*3+1] == false) continue;
                if (idx5 == idx1) {
                  p = vld1q_s32(path);
                  vst1q_s32(ret_[1]+ret_num_[1]*4, p);
                  ret_num_[1]++;
                  continue;
                }
                if (status_map_[idx5*3] == false) {
                  path[4] = idx5;
                  status_map_[idx5*3] = true;

                  for (int m = 0; m < out_degrees_[idx5]; m++) {
                    idx6 = G_[idx5*50+m];
                    if (status_map_[idx6*3+1] == false) continue;
                    if (idx6 == idx1) {
                      p = vld1q_s32(path);
                      vst1q_s32(ret_[2]+ret_num_[2]*8, p);
                      p = vld1q_s32(path+4);
                      vst1q_s32(ret_[2]+ret_num_[2]*8+4, p);
                      ret_num_[2]++;
                      continue;
                    }

		    if (status_map_[idx6*3] == false) {
                      path[5] = idx6;
                      status_map_[idx6*3] = true;
                      
                      for (int n = 0; n < out_degrees_[idx6]; n++) {
                        idx7 = G_[idx6*50+n];
                        if (status_map_[idx7*3+1] == false) continue;
                        if (idx7 == idx1) {
                          p = vld1q_s32(path);
                          vst1q_s32(ret_[3]+ret_num_[3]*8, p);
                          p = vld1q_s32(path+4);
                          vst1q_s32(ret_[3]+ret_num_[3]*8+4, p);
                          ret_num_[3]++;
			  continue;
                        }
			if (status_map_[idx7*3+2] == true && status_map_[idx7*3] == false) {
                          path[6] = idx7;
                          p = vld1q_s32(path);
                          vst1q_s32(ret_[4]+ret_num_[4]*8, p);
                          p = vld1q_s32(path+4);
                          vst1q_s32(ret_[4]+ret_num_[4]*8+4, p);
                          ret_num_[4]++;
                          continue;
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
    delete[](tail_idxes); delete[](effective_idxes);
  }

  void WriteFile(std::string filename) {
    FILE *fp = fopen(filename.c_str(), "wb");
    if (fp == NULL) {
      printf("file open error\n");
      exit(1);
    }

    fprintf(fp, "%d\n", ret_num_[0]+ret_num_[1]+ret_num_[2]+ret_num_[3]+ret_num_[4]);
    std::string item;
    int bias;
    for (int d = 3; d <= 7; d++) {
      for (int i = 0; i < ret_num_[d-3]; i++) {
        bias = i*ret_step_[d-3];
        for (int j = 0; j < d-1; j++) {
          item = ids_comma_[ret_[d-3][bias+j]];
          fwrite(item.c_str(), item.size(), sizeof(char), fp); 
        }
        item = ids_line_[ret_[d-3][bias+d-1]];
        fwrite(item.c_str(), item.size(), sizeof(char), fp); 
      }
    }
    fclose(fp);
  }

private:
  std::vector<std::string> ids_comma_;
  std::vector<std::string> ids_line_;

  int ids_num_;

  int* G_;
  int* inv_G_;
  bool* status_map_;
  int* in_degrees_;
  int* out_degrees_;
  int* ret3_ = new int[4*500000]; 
  int* ret4_ = new int[4*500000]; 
  int* ret5_ = new int[8*1000000];
  int* ret6_ = new int[8*2000000];
  int* ret7_ = new int[8*3000000];
  int* ret_[5] = {ret3_, ret4_, ret5_, ret6_, ret7_};
  int ret_num_[5] = {0, 0, 0, 0, 0}; 
  int ret_step_[5] = {4, 4, 8, 8, 8}; 
};


int main(int argc, char** argv) {
  // DirectedGraph directed_graph("../data/test_data.txt");
  // DirectedGraph directed_graph("../data/HWcode2020-TestData/testData/test_data.txt");
  // DirectedGraph directed_graph("/data/test_data.txt");
  DirectedGraph directed_graph("/root/2020HuaweiCodecraft-TestData/1004812/test_data.txt");

  directed_graph.FindAllCycles();

  directed_graph.WriteFile("go.txt");
  // directed_graph.WriteFile("/projects/student/result.txt");
  // exit(0);

  return 0;
}
