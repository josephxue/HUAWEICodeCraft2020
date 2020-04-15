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

  ~DirectedGraph() {}

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
    for (int start_idx = ids_num_-1; start_idx >= 0; start_idx--) {
      status_map_[start_idx] = true;

      for (auto& tmp : memory_[start_idx]) {
        int local_first_idx = tmp.first;
        if (local_first_idx > start_idx) {
          reachable_[local_first_idx] = true;
          local_first_idxs.emplace_back(local_first_idx);
        }
      }

      for (int& middle_idx : adjacency_list_[start_idx]) {
        if (middle_idx < start_idx) break;
        status_map_[middle_idx] = true;
        for (int& last_idx : adjacency_list_[middle_idx]) {
          if (last_idx <= start_idx) break;
          status_map_[last_idx] = true;
          for (int& x : adjacency_list_[last_idx]) {
            path = {start_idx, middle_idx, last_idx};
            if (x == start_idx) {
              ret_[0].emplace_back(path);
            }
            if (x > start_idx && status_map_[x] == false) {
              DepthFirstSearch(x, start_idx, path, 4);
            }
            path.clear();
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

    fprintf(fp, "%ld\n", ret_[0].size()+ret_[1].size()+ret_[2].size()+ret_[3].size()+ret_[4].size());
    std::string item;
    std::vector<int> path;
    for (std::vector<std::vector<int> >& r : ret_) {
      for (int i = r.size()-1; i >= 0; i--) {
        path = r[i];
        for (int j = 0; j < path.size()-1; j++) {
          item = ids_comma_[path[j]];
          fwrite(item.c_str(), item.size(), sizeof(char), fp);
        }
        item = ids_line_[path[path.size()-1]];
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
  std::vector<int> out_degrees_;
  std::vector<std::vector<int> > ret_[5];

  void ConstructAdjacencyList(std::unordered_map<int, int>& m, std::vector<int>& inputs) {
    adjacency_list_ = std::vector<std::vector<int> >(ids_num_);
    in_degrees_ = std::vector<int>(ids_num_, 0);
    out_degrees_ = std::vector<int>(ids_num_, 0);

    int from = -1; int to = -1;
    for (int i = 0; i < inputs.size(); i+=2) {
      from = m[inputs[i]]; to = m[inputs[i+1]];
      adjacency_list_[from].emplace_back(to);
      out_degrees_[from]++;
      in_degrees_[to]++;
    }

    TopoSort(in_degrees_, false);
    TopoSort(out_degrees_, true);
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
      else if (do_sorting) std::sort(adjacency_list_[i].begin(), adjacency_list_[i].end(), std::greater<int>());
    }
  }

  void DepthFirstSearch(const int& idx, const int& first_idx, std::vector<int>& path, int depth) {
    status_map_[idx] = true;
    path.emplace_back(idx);
    for (int& next_idx : adjacency_list_[idx]) {
      if (next_idx < first_idx) break;
      if (next_idx == first_idx && depth <= 5) {
        ret_[depth-3].emplace_back(path);
      }
      if (next_idx != first_idx && depth == 5) {
        if (reachable_[next_idx] == true && status_map_[next_idx] == false) {
          path.emplace_back(next_idx);
          bool have_minus = false;
          for (int& middle_idx : memory_[first_idx][next_idx]) {
            if (middle_idx > 0 && status_map_[middle_idx] == false) {
              path.emplace_back(middle_idx);
              ret_[4].emplace_back(path);
              path.pop_back();
            }
            if (middle_idx == -1) ret_[3].emplace_back(path); 
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
  DirectedGraph directed_graph("/data/test_data.txt");
  // DirectedGraph directed_graph("/root/2020HuaweiCodecraft-TestData/1004812/test_data.txt");

  directed_graph.FindAllCycles();

  // directed_graph.WriteFile("go.txt");
  directed_graph.WriteFile("/projects/student/result.txt");

  return 0;
}