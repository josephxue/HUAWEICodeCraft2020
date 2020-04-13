#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>


class DirectedGraph {
public:
  DirectedGraph(std::string filename) {
    std::vector<int> transfer_from_ids;
    std::vector<int> transfer_to_ids;

    std::ifstream infile(filename.c_str());
    int transfer_from_id, transfer_to_id, val;
    char comma;
    while (infile >> transfer_from_id >> comma >> transfer_to_id >> comma >> val) {
      transfer_from_ids.emplace_back(transfer_from_id);
      transfer_to_ids.emplace_back(transfer_to_id);
    }
    infile.close();

    std::vector<int> ids;
    std::vector<int> tmp_from = transfer_from_ids;
    std::vector<int> tmp_to = transfer_to_ids;
    std::sort(tmp_from.begin(), tmp_from.end());
    std::sort(tmp_to.begin(), tmp_to.end());
    std::set_intersection(
        tmp_from.begin(), tmp_from.end(), 
        tmp_to.begin(), tmp_to.end(),
        std::back_inserter(ids));
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    ids_ = ids;

    adjacency_list_ = std::vector<std::vector<int> >(ids_.size());
    std::unordered_map<int, int> m;
    for (int i = 0; i < ids.size(); i++) {
      m[ids_[i]] = i;
    }

    ConstructAdjacencyList(m, transfer_from_ids, transfer_to_ids);
  }

  ~DirectedGraph() {}

  void FindAllCycles(std::vector<std::vector<int> >& ret) {
    std::vector<bool> status_map(adjacency_list_.size(), false);

    std::vector<std::unordered_map<int, std::vector<int>>> memory(adjacency_list_.size());
    int middle_idx = -1, last_idx = -1;
    for (int i = 0; i < adjacency_list_.size(); i++) {
      for (int j = 0; j < adjacency_list_[i].size(); j++) {
        middle_idx = adjacency_list_[i][j];
        if (i > middle_idx)
          memory[i][middle_idx].emplace_back(-1);
        for (int k = 0; k < adjacency_list_[middle_idx].size(); k++) {
          last_idx = adjacency_list_[middle_idx][k];
          if (i > last_idx && middle_idx > last_idx)
            memory[i][last_idx].emplace_back(middle_idx);
        }
      }
    }

    std::vector<int> path;
    for (int i = 0; i < status_map.size(); i++) {
      DepthFirstSearch(i, i, path, ret, 1, status_map, memory);
    }
    std::sort(ret.begin(), ret.end(), Compare);
  }

private:
  std::vector<int> ids_;
  std::vector<std::vector<int> > adjacency_list_;

  void ConstructAdjacencyList(std::unordered_map<int, int>& m,
      std::vector<int>& transfer_from_ids, std::vector<int>& transfer_to_ids) {
    for (int i = 0; i < transfer_from_ids.size(); i++) {
      if (m.find(transfer_from_ids[i]) != m.end() && m.find(transfer_to_ids[i]) != m.end()) {
        adjacency_list_[m[transfer_from_ids[i]]].emplace_back(m[transfer_to_ids[i]]);
      }
    }
    for (int i = 0; i < adjacency_list_.size(); i++) {
      std::sort(adjacency_list_[i].begin(), adjacency_list_[i].end(), std::greater<int>());
    }
  }

  void DepthFirstSearch(const int& idx, const int& first_idx, 
      std::vector<int>& path, std::vector<std::vector<int> >& ret, int depth,
      std::vector<bool>& status_map, std::vector<std::unordered_map<int, std::vector<int>>>& memory) {
    status_map[idx] = true;
    path.emplace_back(ids_[idx]);
    for (int& next_idx : adjacency_list_[idx]) {
      if (next_idx < first_idx) break;
      if (next_idx == first_idx && depth >= 3 && depth <= 5) {
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
  DirectedGraph directed_graph("/data/test_data.txt");
  // DirectedGraph directed_graph("../data/2020HuaweiCodecraft-TestData/1004812/test_data.txt");

  std::vector<std::vector<int> > ret;
  directed_graph.FindAllCycles(ret);

  // std::ofstream outfile("go.txt", std::ios::out);
  std::ofstream outfile("/projects/student/result.txt", std::ios::out);
  outfile << ret.size() << std::endl;
  for (std::vector<int>& path : ret) {
    for (int i = 0; i < path.size()-1; i++)
      outfile << path[i] << ",";
    outfile << path[path.size()-1] << std::endl;
  }
  outfile.close();

  return 0;
}