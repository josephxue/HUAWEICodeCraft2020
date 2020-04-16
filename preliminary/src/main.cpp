#include <stdio.h>

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

    ConstructAdjacencyList(m, inputs);
  }

  ~DirectedGraph() {
    delete[](ret3_); delete[](ret4_); delete[](ret5_); delete[](ret6_); delete[](ret7_);
  }

  void FindAllCycles() {
    status_map_ = std::vector<bool>(ids_num_, false);
    reachable_ = std::vector<bool>(ids_num_, false);

    memory_ = std::vector<std::unordered_map<int, std::vector<int>>>(adjacency_list_.size());
    for (int start_idx = 0; start_idx < ids_num_; start_idx++) {
      for (int& middle_idx : adjacency_list_[start_idx]) {
        if (start_idx > middle_idx) memory_[middle_idx][start_idx].emplace_back(-1);
        for (int& last_idx : adjacency_list_[middle_idx]) {
          if (start_idx > last_idx && middle_idx > last_idx)
            memory_[last_idx][start_idx].emplace_back(middle_idx);
        }
      }
    }

    std::vector<int> path;
    std::vector<int> local_first_idxs;
    for (int start_idx = 0; start_idx < ids_num_; start_idx++) {
      status_map_[start_idx] = true;

      for (auto& tmp : memory_[start_idx]) {
        int local_first_idx = tmp.first;
        if (local_first_idx > start_idx) {
          reachable_[local_first_idx] = true;
          local_first_idxs.emplace_back(local_first_idx);
        }
      }

      for (int& middle_idx : adjacency_list_[start_idx]) {
        if (middle_idx < start_idx) continue;
        status_map_[middle_idx] = true;
        for (int& last_idx : adjacency_list_[middle_idx]) {
          if (last_idx <= start_idx) continue;
          status_map_[last_idx] = true;
          for (int& x : adjacency_list_[last_idx]) {
            if (x == start_idx) {
              ret_[0][ret_num_[0]*ret_step_[0]]   = start_idx;
              ret_[0][ret_num_[0]*ret_step_[0]+1] = middle_idx;
              ret_[0][ret_num_[0]*ret_step_[0]+2] = last_idx;
              ret_num_[0]++;
              continue;
            }
            if (x > start_idx && status_map_[x] == false) {
              path = {start_idx, middle_idx, last_idx};
              DepthFirstSearch(x, start_idx, path, 4);
              path.clear();
            }
          }
          status_map_[last_idx] = false;
        }
        status_map_[middle_idx] = false;
      }
      status_map_[start_idx] = false;

      for (int& local_first_idx : local_first_idxs)
        reachable_[local_first_idx] = false;
      local_first_idxs.clear();
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
    std::vector<int> path;
    for (int d = 3; d <= 7; d++) {
      for (int i = 0; i < ret_num_[d-3]; i++) {
        for (int j = 0; j < d-1; j++) {
          item = ids_comma_[ret_[d-3][i*ret_step_[d-3]+j]];
          fwrite(item.c_str(), item.size(), sizeof(char), fp); 
        }
        item = ids_line_[ret_[d-3][i*ret_step_[d-3]+d-1]];
        fwrite(item.c_str(), item.size(), sizeof(char), fp); 
      }
    }
    fclose(fp);
  }

private:
  std::vector<std::string> ids_comma_;
  std::vector<std::string> ids_line_;
  int ids_num_;

  std::vector<std::vector<int> > adjacency_list_;
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

  void ConstructAdjacencyList(std::unordered_map<int, int>& m, std::vector<int>& inputs) {
    adjacency_list_ = std::vector<std::vector<int> >(ids_num_);
    in_degrees_ = std::vector<int>(ids_num_, 0);

    int from = -1; int to = -1;
    for (int i = 0; i < inputs.size(); i+=2) {
      from = m[inputs[i]]; to = m[inputs[i+1]];
      adjacency_list_[from].emplace_back(to);
      in_degrees_[to]++;
    }

    TopoSort(in_degrees_, true);
  }

  void TopoSort(std::vector<int>& degrees, bool do_sorting) {
    std::queue<int> q;
    for (int i = 0; i < ids_num_; i++)
      if (0 == degrees[i]) q.push(i);

    int u = -1;
    while (!q.empty()) {
      u = q.front();
      q.pop();
      for (int& v : adjacency_list_[u]) {
        if (0 == --degrees[v]) q.push(v);
      }
    }

    for (int i = 0; i < ids_num_; i++) {
      if (degrees[i] == 0) adjacency_list_[i].clear();
      else if (do_sorting) std::sort(adjacency_list_[i].begin(), adjacency_list_[i].end());
    }
  }

  void DepthFirstSearch(const int& idx, const int& first_idx, std::vector<int>& path, int depth) {
    status_map_[idx] = true;
    path.emplace_back(idx);
    for (int& next_idx : adjacency_list_[idx]) {
      if (next_idx < first_idx) continue;
      if (next_idx == first_idx && depth <= 5) {
        for (int k = 0; k < depth; k++) 
          ret_[depth-3][ret_num_[depth-3]*ret_step_[depth-3]+k] = path[k];
        ret_num_[depth-3]++;
      }
      if (next_idx != first_idx && depth == 5) {
        if (reachable_[next_idx] == true && status_map_[next_idx] == false) {
          path.emplace_back(next_idx);
          for (int& middle_idx : memory_[first_idx][next_idx]) {
            if (middle_idx > 0 && status_map_[middle_idx] == false) {
              path.emplace_back(middle_idx);
              for (int k = 0; k < 7; k++)
                ret_[4][ret_num_[4]*ret_step_[depth-3]+k] = path[k];
              ret_num_[4]++;
              path.pop_back();
            }
            if (middle_idx == -1) {
              for (int k = 0; k < 6; k++)
                ret_[3][ret_num_[3]*ret_step_[depth-3]+k] = path[k];
              ret_num_[3]++;
            }
          }
          path.pop_back();
        }
      }
      if (depth < 5 && status_map_[next_idx] == false) {
        DepthFirstSearch(next_idx, first_idx, path, depth+1);
      }
    }
    path.pop_back();
    status_map_[idx] = false;
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