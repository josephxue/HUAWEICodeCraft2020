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
    int ret;
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

  void FindAllCycles(std::vector<std::vector<int> >& ret) {
    std::vector<bool> status_map(adjacency_list_.size(), false);

    std::vector<std::unordered_map<int, std::vector<int>>> memory(adjacency_list_.size());
    for (int start_idx = 0; start_idx < adjacency_list_.size(); start_idx++) {
      for (int& middle_idx : adjacency_list_[start_idx]) {
        if (start_idx > middle_idx) memory[start_idx][middle_idx].emplace_back(-1);
        for (int& last_idx : adjacency_list_[middle_idx]) {
          if (start_idx > last_idx && middle_idx > last_idx)
            memory[start_idx][last_idx].emplace_back(middle_idx);
        }
      }
    }

    std::vector<int> path;
    for (int start_idx = 0; start_idx < adjacency_list_.size(); start_idx++) {
      for (int& middle_idx : adjacency_list_[start_idx]) {
        if (middle_idx < start_idx) break;
        for (int& last_idx : adjacency_list_[middle_idx]) {
          if (last_idx <= start_idx) break;
          status_map[start_idx] = true, status_map[middle_idx] = true, status_map[last_idx] = true;
          path = {ids_[start_idx], ids_[middle_idx], ids_[last_idx]};
          for (int& x : adjacency_list_[last_idx]) {
            if (x == start_idx) ret.emplace_back(path);
            if (x > start_idx && status_map[x] == false) {
              DepthFirstSearch(x, start_idx, path, ret, 4, status_map, memory);
            }
          }
          path.clear();
          status_map[start_idx] = false, status_map[middle_idx] = false, status_map[last_idx] = false;
        }
      }
    }

    std::sort(ret.begin(), ret.end(), Compare);
  }

private:
  std::vector<int> ids_;
  std::vector<std::vector<int> > adjacency_list_;

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
      std::sort(adjacency_list_[i].begin(), adjacency_list_[i].end(), std::greater<int>());
  }

  void DepthFirstSearch(const int& idx, const int& first_idx, 
      std::vector<int>& path, std::vector<std::vector<int> >& ret, int depth,
      std::vector<bool>& status_map, std::vector<std::unordered_map<int, std::vector<int>>>& memory) {
    status_map[idx] = true;
    path.emplace_back(ids_[idx]);
    for (int& next_idx : adjacency_list_[idx]) {
      if (next_idx < first_idx) break;
      if (next_idx == first_idx && depth <= 5) {
        ret.emplace_back(path);
      }
      if (next_idx != first_idx && depth == 5) {
        std::unordered_map<int, std::vector<int>>::iterator it = memory[next_idx].find(first_idx);
        if (it != memory[next_idx].end() && status_map[next_idx] == false) {
          path.emplace_back(ids_[next_idx]);
          bool have_minus = false;
          for (int& middle_idx : it->second) {
            if (middle_idx > 0 && status_map[middle_idx] == false) {
              path.emplace_back(ids_[middle_idx]);
              ret.emplace_back(path);
              path.pop_back();
            }
            if (middle_idx == -1) ret.emplace_back(path); 
          }
          path.pop_back();
        }
      }
      if (depth < 5 && status_map[next_idx] == false) {
        DepthFirstSearch(next_idx, first_idx, path, ret, depth+1, status_map, memory);
      }
    }
    path.pop_back();
    status_map[idx] = false;
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
  // DirectedGraph directed_graph("/data/test_data.txt");
  DirectedGraph directed_graph("/root/2020HuaweiCodecraft-TestData/1004812/test_data.txt");

  std::vector<std::vector<int> > ret;
  directed_graph.FindAllCycles(ret);

  FILE *fp = fopen("go.txt", "w");
  // FILE *fp = fopen("/projects/student/result.txt", "r");
  if (fp == NULL) {
    printf("file open error\n");
    exit(1);
  }

  fprintf(fp, "%ld\n", ret.size());
  for (std::vector<int>& path : ret) {
    for (int i = 0; i < path.size()-1; i++)
      fprintf(fp, "%d,", path[i]);
    fprintf(fp, "%d\n", path[path.size()-1]);
  }
  fclose(fp);
  

  // std::ofstream outfile("go.txt", std::ios::out);
  // // std::ofstream outfile("/projects/student/result.txt", std::ios::out);
  // outfile << ret.size() << std::endl;
  // for (std::vector<int>& path : ret) {
  //   for (int i = 0; i < path.size()-1; i++)
  //     outfile << path[i] << ",";
  //   outfile << path[path.size()-1] << std::endl;
  // }
  // outfile.close();

  return 0;
}