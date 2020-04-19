#include <stdio.h>
#include <string.h>
#include <arm_neon.h>

#include <queue>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
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
    int transfer_from_id, transfer_to_id, val;
    while (fscanf(fp, "%d,%d,%d", &transfer_from_id, &transfer_to_id, &val) > 0) {
      inputs[inputs_size] = transfer_from_id;
      inputs[inputs_size+1] = transfer_to_id;
      inputs_size += 2;
    }
    fclose(fp);

    std::vector<int> ids(inputs, inputs+inputs_size);
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    ids_num_ = ids.size();

    std::unordered_map<int, int> m;

    ids_comma_.reserve(ids_num_);
    ids_line_.reserve(ids_num_);

    int id;
    for (int i = 0; i < ids_num_; i++) {
      id = ids[i];
      ids_comma_.emplace_back(std::to_string(id) + ',');
      ids_line_.emplace_back(std::to_string(id) + '\n');
      m[id] = i;
    }

    G_ = new int[280000*50];
    in_degrees_  = new int[ids_num_]; memset(in_degrees_,  0, ids_num_*sizeof(int));
    out_degrees_ = new int[ids_num_]; memset(out_degrees_, 0, ids_num_*sizeof(int));

    int from = -1; int to = -1;
    for (int i = 0; i < inputs_size; i+=2) {
      from = m[inputs[i]]; to = m[inputs[i+1]];
      G_[from*50+out_degrees_[from]] = to;
      in_degrees_[to]++;
      out_degrees_[from]++;
    }
    delete[](inputs);
    TopoSort(in_degrees_, true);
  }

  ~DirectedGraph() {
    delete[](ret3_); delete[](ret4_); delete[](ret5_); delete[](ret6_); delete[](ret7_);
    delete[](in_degrees_); delete[](out_degrees_);
    delete[](G_);
    delete[](status_map_);
    delete[](reachable_);
  }

  void FindAllCycles() {
    int32x4_t p;

    status_map_ = new bool[ids_num_]; memset(status_map_, false, ids_num_*sizeof(bool));
    reachable_  = new bool[ids_num_]; memset(reachable_,  false, ids_num_*sizeof(bool));

    int idx1, idx2, idx3, idx4, idx5, idx6, idx7;

    memory_ = std::vector<std::unordered_map<int, std::vector<int>>>(ids_num_);
    for (idx1 = 0; idx1 < ids_num_; idx1++) {
      for (int i = 0; i < out_degrees_[idx1]; i++) {
        idx2 = G_[idx1*50+i];
        if (idx1 > idx2) memory_[idx2][idx1].emplace_back(-1);
        for (int j = 0; j < out_degrees_[idx2]; j++) {
          idx3 = G_[idx2*50+j];
          if (idx1 > idx3 && idx2 > idx3)
            memory_[idx3][idx1].emplace_back(idx2);
        }
      }
    }

    int local_idx1;
    std::vector<int> local_idxs1;
    int bias;
    int path[8];
    for (idx1 = 0; idx1 < ids_num_; idx1++) {
      if (out_degrees_[idx1] == 0) continue;
      if (G_[idx1*50+out_degrees_[idx1]-1] < idx1) continue;
      path[0] = idx1;

      for (auto& tmp : memory_[idx1]) {
        local_idx1 = tmp.first;
        if (local_idx1 > idx1) {
          reachable_[local_idx1] = true;
          local_idxs1.emplace_back(local_idx1);
        }
      }

      for (int i = 0; i < out_degrees_[idx1]; i++) {
        idx2 = G_[idx1*50+i];
        if (idx2 < idx1 || out_degrees_[idx2] == 0) continue;
        if (G_[idx2*50+out_degrees_[idx2]-1] < idx1) continue;
        path[1] = idx2;
        status_map_[idx2] = true;

        for (int j = 0; j < out_degrees_[idx2]; j++) {
          idx3 = G_[idx2*50+j];
          if (idx3 <= idx1) continue;
          path[2] = idx3;
          status_map_[idx3] = true;

          for (int k = 0; k < out_degrees_[idx3]; k++) {
            idx4 = G_[idx3*50+k];
            if (idx4 < idx1) continue;
            if (idx4 == idx1) {
              p = vld1q_s32(path);
              vst1q_s32(ret_[0]+ret_num_[0]*4, p);
              ret_num_[0]++;
              continue;
            }
            if (status_map_[idx4] == false) {
              path[3] = idx4;
              status_map_[idx4] = true;

              for (int l = 0; l < out_degrees_[idx4]; l++) {
                idx5 = G_[idx4*50+l];
                if (idx5 < idx1) continue;
                if (idx5 == idx1) {
                  p = vld1q_s32(path);
                  vst1q_s32(ret_[1]+ret_num_[1]*4, p);
                  ret_num_[1]++;
                  continue;
                }
                if (status_map_[idx5] == false) {
                  path[4] = idx5;
                  status_map_[idx5] = true;

                  for (int m = 0; m < out_degrees_[idx5]; m++) {
                    idx6 = G_[idx5*50+m];
                    if (idx6 < idx1) continue;
                    if (idx6 == idx1) {
                      p = vld1q_s32(path);
                      vst1q_s32(ret_[2]+ret_num_[2]*8, p);
                      p = vld1q_s32(path+4);
                      vst1q_s32(ret_[2]+ret_num_[2]*8+4, p);
                      ret_num_[2]++;
                      continue;
                    }
                    if (reachable_[idx6] == true && status_map_[idx6] == false) {
                      path[5] = idx6;
                      for (int& idx7 : memory_[idx1][idx6]) {
                        if (idx7 > 0 && status_map_[idx7] == false) {
                          path[6] = idx7;
                          p = vld1q_s32(path);
                          vst1q_s32(ret_[4]+ret_num_[4]*8, p);
                          p = vld1q_s32(path+4);
                          vst1q_s32(ret_[4]+ret_num_[4]*8+4, p);
                          ret_num_[4]++;
                          continue;
                        }
                        if (idx7 == -1) {
                          p = vld1q_s32(path);
                          vst1q_s32(ret_[3]+ret_num_[3]*8, p);
                          p = vld1q_s32(path+4);
                          vst1q_s32(ret_[3]+ret_num_[3]*8+4, p);
                          ret_num_[3]++;
                        }
                      }
                    }
                  }
                  status_map_[idx5] = false;
                }
              }
              status_map_[idx4] = false;
            }
          }
          status_map_[idx3] = false;
        }
        status_map_[idx2] = false;
      }

      for (int& local_idx1 : local_idxs1)
        reachable_[local_idx1] = false;
      local_idxs1.clear();
    }
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
  std::vector<std::unordered_map<int, std::vector<int>>> memory_;
  bool* status_map_;
  bool* reachable_;
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

  void TopoSort(int* degrees, bool do_sorting) {
    std::queue<int> q;
    for (int i = 0; i < ids_num_; i++)
      if (0 == degrees[i]) q.push(i);

    int u, v;
    while (!q.empty()) {
      u = q.front();
      q.pop();
      for (int i = 0; i < out_degrees_[u]; i++) {
        v = G_[u*50+i];
        if (0 == --degrees[v]) q.push(v);
      }
    }

    for (int i = 0; i < ids_num_; i++) {
      if (degrees[i] == 0) out_degrees_[i] = 0;
      else if (do_sorting) std::sort(G_+i*50, G_+i*50+out_degrees_[i]);
    }
  }
};


int main(int argc, char** argv) {
  // DirectedGraph directed_graph("../data/test_data.txt");
  // DirectedGraph directed_graph("../data/HWcode2020-TestData/testData/test_data.txt");
  DirectedGraph directed_graph("/data/test_data.txt");
  // DirectedGraph directed_graph("/root/2020HuaweiCodecraft-TestData/1004812/test_data.txt");

  directed_graph.FindAllCycles();

  // directed_graph.WriteFile("go.txt");
  directed_graph.WriteFile("/projects/student/result.txt");
  exit(0);

  return 0;
}