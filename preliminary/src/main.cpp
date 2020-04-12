#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>


class DirectedGraph {
public:
  DirectedGraph(std::string filename) {
    std::ifstream infile(filename.c_str());
    int transfer_from_id, transfer_to_id, val;
    char comma;
    while (infile >> transfer_from_id >> comma >> transfer_to_id >> comma >> val) {
      transfer_from_ids_.push_back(transfer_from_id);
      transfer_to_ids_.push_back(transfer_to_id);
    }
    infile.close();

    std::vector<int> ids;
    std::vector<int> tmp_from = transfer_from_ids_;
    std::vector<int> tmp_to = transfer_to_ids_;
    std::sort(tmp_from.begin(), tmp_from.end());
    std::sort(tmp_to.begin(), tmp_to.end());
    std::set_intersection(
        tmp_from.begin(), tmp_from.end(), 
        tmp_to.begin(), tmp_to.end(),
        std::back_inserter(ids));
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    ids_ = ids;

    adjacency_list_ = std::vector<std::vector<int> >(ids_.size());
    incoming_adjacency_list_ = std::vector<std::vector<int> >(ids_.size());
    for (int i = 0; i < ids.size(); i++) {
      m_[ids_[i]] = i;
    }

    ConstructAdjacencyList();
  }

  ~DirectedGraph() {}

  std::vector<std::vector<int> > FindAllCycles(std::vector<std::vector<int> >* ret) {
    // map to represent the status of each vertex
    // 1 represents the vertex has already been visited
    // 0 represents the vertex has not been visited
    // -1 represents the vertex can be skipped(not envovled in the ids or finished processed)
    // std::vector<int> status_map(adjacency_list_.size(), 0);
    std::vector<bool> status_map(adjacency_list_.size(), false);
    std::vector<int> path;
    for (int i = 0; i < status_map.size(); i++) {
      status_map[i] = true;
      path.push_back(ids_[i]);
      DepthFirstSearch(i, i, &path, &status_map, ret);
      status_map[i] = false;
      path.pop_back();
    }
    std::sort(ret->begin(), ret->end(), Compare);

    return *ret;
  }

private:
  std::vector<int> transfer_from_ids_;
  std::vector<int> transfer_to_ids_;

  std::vector<std::vector<int> > adjacency_list_;
  std::vector<std::vector<int> > incoming_adjacency_list_;
  std::vector<int> ids_;
  std::unordered_map<int, int> m_;

  void ConstructAdjacencyList() {
    for (int i = 0; i < transfer_from_ids_.size(); i++) {
      if (m_.find(transfer_from_ids_[i]) != m_.end() && m_.find(transfer_to_ids_[i]) != m_.end()) {
        adjacency_list_[m_[transfer_from_ids_[i]]].push_back(m_[transfer_to_ids_[i]]);
        incoming_adjacency_list_[m_[transfer_to_ids_[i]]].push_back(m_[transfer_from_ids_[i]]);
      }
    }
    for (int i = 0; i < adjacency_list_.size(); i++) {
      std::sort(adjacency_list_[i].begin(), adjacency_list_[i].end(), std::greater<int>());
    }
  }

  void DepthFirstSearch(int idx, int first_idx, std::vector<int>* path, std::vector<bool>* status_map, 
      std::vector<std::vector<int> >* ret) {
    for (int i = 0; i < adjacency_list_[idx].size(); i++) {
      if (adjacency_list_[idx][i] < first_idx) break;

      if ((*status_map)[adjacency_list_[idx][i]] == true) {
        if (adjacency_list_[idx][i] == first_idx && path->size() >= 3)
          ret->push_back(*path);
      } else {
        (*status_map)[adjacency_list_[idx][i]] = true;
        path->push_back(ids_[adjacency_list_[idx][i]]);
        if (path->size() == 6) {
          int current_idx = adjacency_list_[idx][i];
          if (std::find(incoming_adjacency_list_[first_idx].begin(), incoming_adjacency_list_[first_idx].end(), current_idx)
              != incoming_adjacency_list_[first_idx].end()) {
            ret->push_back(*path);
          }

          int next_idx = adjacency_list_[idx][i];
          for (int j = 0; j < adjacency_list_[next_idx].size(); j++) {
            if (adjacency_list_[next_idx][j] < first_idx) break;
            if (std::find(incoming_adjacency_list_[first_idx].begin(), incoming_adjacency_list_[first_idx].end(), adjacency_list_[next_idx][j]) 
                != incoming_adjacency_list_[first_idx].end() && (*status_map)[adjacency_list_[next_idx][j]] == 0) {
              path->push_back(ids_[adjacency_list_[next_idx][j]]);
              ret->push_back(*path);
              path->pop_back();
            }
          }
        } else {
          DepthFirstSearch(adjacency_list_[idx][i], first_idx, path, status_map, ret);
        }
        (*status_map)[adjacency_list_[idx][i]] = false;
        path->pop_back();
      }
    }
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

  std::vector<std::vector<int> > all_cycles;
  std::vector<std::vector<int> > ret;
  ret = directed_graph.FindAllCycles(&all_cycles);

  // std::ofstream outfile("go.txt", std::ios::out);
  std::ofstream outfile("/projects/student/result.txt", std::ios::out);
  outfile << ret.size() << std::endl;
  for (int i = 0; i < ret.size(); i++) {
    for (int j = 0; j < ret[i].size()-1; j++) {
      outfile << ret[i][j] << ",";
    }
    outfile << ret[i][ret[i].size()-1] << std::endl;
  }
  outfile.close();

  return 0;
}