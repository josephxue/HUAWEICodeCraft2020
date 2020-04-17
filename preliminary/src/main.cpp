#include <stdio.h>
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

    std::vector<int> inputs;
    inputs.reserve(560000);
    int transfer_from_id, transfer_to_id, val;
    while (fscanf(fp, "%d,%d,%d", &transfer_from_id, &transfer_to_id, &val) > 0) {
      inputs.emplace_back(transfer_from_id);
      inputs.emplace_back(transfer_to_id);
    }
    fclose(fp);

    std::vector<int> ids = inputs;
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

    ids_num_ = 0; 
    std::unordered_map<int, int> m;
    ids_comma_.reserve(ids.size());
    ids_line_.reserve(ids.size());
    for (int& id : ids) {
      ids_comma_.emplace_back(std::to_string(id) + ',');
      ids_line_.emplace_back(std::to_string(id) + '\n');
      m[id] = ids_num_++;
    }

    std::vector<int> tmp; tmp.reserve(50);
    G_ = std::vector<std::vector<int> >(ids_num_, tmp);
    in_degrees_ = std::vector<int>(ids_num_, 0);

    int from = -1; int to = -1;
    for (int i = 0; i < inputs.size(); i+=2) {
      from = m[inputs[i]]; to = m[inputs[i+1]];
      G_[from].emplace_back(to);
      in_degrees_[to]++;
    }
    TopoSort(in_degrees_, true);
  }

  ~DirectedGraph() {
    delete[](ret3_); delete[](ret4_); delete[](ret5_); delete[](ret6_); delete[](ret7_);
  }

  void FindAllCycles() {
    int32x4_t p;

    status_map_ = std::vector<bool>(ids_num_, false);
    reachable_ = std::vector<bool>(ids_num_, false);

    memory_ = std::vector<std::unordered_map<int, std::vector<int>>>(G_.size());
    for (int idx1 = 0; idx1 < ids_num_; idx1++) {
      for (int& idx2 : G_[idx1]) {
        if (idx1 > idx2) memory_[idx2][idx1].emplace_back(-1);
        for (int& idx3 : G_[idx2]) {
          if (idx1 > idx3 && idx2 > idx3)
            memory_[idx3][idx1].emplace_back(idx2);
        }
      }
    }

    int local_idx1;
    std::vector<int> local_idxs1;
    int bias;
    int path[8];
    for (int idx1 = 0; idx1 < ids_num_; idx1++) {
      if (G_[idx1].empty()) continue;
      if (G_[idx1][G_[idx1].size()-1] < idx1) continue;
      path[0] = idx1;

      for (auto& tmp : memory_[idx1]) {
        local_idx1 = tmp.first;
        if (local_idx1 > idx1) {
          reachable_[local_idx1] = true;
          local_idxs1.emplace_back(local_idx1);
        }
      }

      for (int& idx2 : G_[idx1]) {
        if (idx2 < idx1 || G_[idx2].empty()) continue;
        if (G_[idx2][G_[idx2].size()-1] < idx1) continue;
        path[1] = idx2;
        status_map_[idx2] = true;

        for (int& idx3 : G_[idx2]) {
          if (idx3 <= idx1) continue;
          path[2] = idx3;
          status_map_[idx3] = true;

          for (int& idx4 : G_[idx3]) {
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

              for (int& idx5 : G_[idx4]) {
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

                  for (int& idx6 : G_[idx5]) {
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

  std::vector<std::vector<int> > G_;
  std::vector<std::unordered_map<int, std::vector<int>>> memory_;
  std::vector<bool> status_map_;
  std::vector<bool> reachable_;
  std::vector<int> in_degrees_;
  int* ret3_ = new int[4*500000]; 
  int* ret4_ = new int[4*500000]; 
  int* ret5_ = new int[8*1000000];
  int* ret6_ = new int[8*2000000];
  int* ret7_ = new int[8*3000000];
  int* ret_[5] = {ret3_, ret4_, ret5_, ret6_, ret7_};
  int ret_num_[5] = {0, 0, 0, 0, 0}; 
  int ret_step_[5] = {4, 4, 8, 8, 8}; 

  void TopoSort(std::vector<int>& degrees, bool do_sorting) {
    std::queue<int> q;
    for (int i = 0; i < ids_num_; i++)
      if (0 == degrees[i]) q.push(i);

    int u = -1;
    while (!q.empty()) {
      u = q.front();
      q.pop();
      for (int& v : G_[u]) {
        if (0 == --degrees[v]) q.push(v);
      }
    }

    for (int i = 0; i < ids_num_; i++) {
      if (degrees[i] == 0) G_[i].clear();
      else if (do_sorting) std::sort(G_[i].begin(), G_[i].end());
    }
  }
};


int main(int argc, char** argv) {
  // DirectedGraph directed_graph("../data/test_data.txt");
  // DirectedGraph directed_graph("../data/HWcode2020-TestData/testData/test_data.txt");
  // DirectedGraph directed_graph("/data/test_data.txt");
  DirectedGraph directed_graph("/root/2020HuaweiCodecraft-TestData/1004812/test_data.txt");

  directed_graph.FindAllCycles();

  directed_graph.WriteFile("go.txt");
  // directed_graph.WriteFile("/projects/student/result.txt");

  return 0;
}