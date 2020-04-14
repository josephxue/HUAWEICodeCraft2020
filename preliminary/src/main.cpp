#include <stdio.h>

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

    ids_ = inputs;
    std::sort(ids_.begin(), ids_.end());
    ids_.erase(std::unique(ids_.begin(), ids_.end()), ids_.end());

    adjacency_list_ = std::vector<std::vector<int> >(ids_.size());
    std::unordered_map<int, int> m;
    for (int i = 0; i < ids_.size(); i++) {
      m[ids_[i]] = i;
    }

    ConstructAdjacencyList(m, inputs);
  }

  ~DirectedGraph() {}

  void FindAllCycles() {
    status_map_ = std::vector<bool>(adjacency_list_.size(), false);
    reachable_ = std::vector<bool>(adjacency_list_.size(), false);

    memory_ = std::vector<std::unordered_map<int, std::vector<int>>>(adjacency_list_.size());
    for (int start_idx = 0; start_idx < adjacency_list_.size(); start_idx++) {
      for (int& middle_idx : adjacency_list_[start_idx]) {
        if (start_idx > middle_idx) memory_[middle_idx][start_idx].emplace_back(-1);
        for (int& last_idx : adjacency_list_[middle_idx]) {
          if (start_idx > last_idx && middle_idx > last_idx)
            memory_[last_idx][start_idx].emplace_back(middle_idx);
        }
      }
    }

    std::vector<int> path;
    for (int start_idx = 0; start_idx < adjacency_list_.size(); start_idx++) {
      status_map_[start_idx] = true;
      path.emplace_back(ids_[start_idx]);

      for (auto& tmp : memory_[start_idx]) {
        int local_first_idx = tmp.first;
        if (local_first_idx > start_idx) {
          reachable_[local_first_idx] = true;
        }
      }

      for (int& middle_idx : adjacency_list_[start_idx]) {
        if (middle_idx < start_idx) continue;
        status_map_[middle_idx] = true;
        path.emplace_back(ids_[middle_idx]);
        for (int& last_idx : adjacency_list_[middle_idx]) {
          if (last_idx <= start_idx) continue;
          status_map_[last_idx] = true;
          path.emplace_back(ids_[last_idx]);
          for (int& x : adjacency_list_[last_idx]) {
            if (x == start_idx) ret_[0].emplace_back(path);
            if (x > start_idx && status_map_[x] == false) {
              DepthFirstSearch(x, start_idx, path, 4);
            }
          }
          path.pop_back();
          status_map_[last_idx] = false;
        }
        path.pop_back();
        status_map_[middle_idx] = false;
      }
      path.pop_back();
      status_map_[start_idx] = false;

      reachable_ = std::vector<bool>(adjacency_list_.size(), false);
    }
  }

  void WriteFile(std::string filename) {
    FILE *fp = fopen(filename.c_str(), "w");
    if (fp == NULL) {
      printf("file open error\n");
      exit(1);
    }

    fprintf(fp, "%ld\n", ret_[0].size()+ret_[1].size()+ret_[2].size()+ret_[3].size()+ret_[4].size());
    for (std::vector<std::vector<int> >& r : ret_) {
      for (std::vector<int>& path : r) {
        for (int i = 0; i < path.size()-1; i++)
          fprintf(fp, "%d,", path[i]);
        fprintf(fp, "%d\n", path[path.size()-1]);
      }
    }
    fclose(fp);
  }

private:
  std::vector<int> ids_;
  std::vector<std::vector<int> > adjacency_list_;
  std::vector<std::unordered_map<int, std::vector<int>>> memory_;
  std::vector<bool> status_map_;
  std::vector<bool> reachable_;
  std::vector<std::vector<int> > ret_[5];

  void ConstructAdjacencyList(std::unordered_map<int, int>& m, std::vector<int>& inputs) {
    std::unordered_map<int, int>::iterator it1;
    std::unordered_map<int, int>::iterator it2;
    for (int i = 0; i < inputs.size(); i+=2) {
      it1 = m.find(inputs[i]), it2 = m.find(inputs[i+1]);
      if (it1 != m.end() && it2 != m.end()) {
        adjacency_list_[it1->second].emplace_back(it2->second);
      }
    }

    for (int i = 0; i < adjacency_list_.size(); i++)
      std::sort(adjacency_list_[i].begin(), adjacency_list_[i].end());
  }

  void DepthFirstSearch(const int& idx, const int& first_idx, std::vector<int>& path, int depth) {
    status_map_[idx] = true;
    path.emplace_back(ids_[idx]);
    for (int& next_idx : adjacency_list_[idx]) {
      if (next_idx < first_idx) continue;
      if (next_idx == first_idx && depth <= 5) {
        ret_[depth-3].emplace_back(path);
      }
      if (next_idx != first_idx && depth == 5) {
        if (reachable_[next_idx] == true && status_map_[next_idx] == false) {
          path.emplace_back(ids_[next_idx]);
          bool have_minus = false;
          for (int& middle_idx : memory_[first_idx][next_idx]) {
            if (middle_idx > 0 && status_map_[middle_idx] == false) {
              path.emplace_back(ids_[middle_idx]);
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

  static bool Compare(std::vector<int> a, std::vector<int> b) {
    if (a.size() == b.size()) {
      for (int i = 0; i < a.size(); i++) {
        if (a[i] == b[i])
          continue;
        return a[i] < b[i];
      }
    } else {
      return a.size() < b.size();
    }
    return false;
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