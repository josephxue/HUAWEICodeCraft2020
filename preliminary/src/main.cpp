#include <cstddef>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>

typedef struct ListNode {
  int val;
  ListNode* next;

  ListNode(int val_) : val(val_) {next = NULL;}
} ListNode;

class DirectedGraph {
public:
  DirectedGraph(std::string filename) {
    std::ifstream infile(filename.c_str());
    int transfer_from_id, transfer_to_id, val;
    char comma;
    max_id_ = 0;
    while (infile >> transfer_from_id >> comma >> transfer_to_id >> comma >> val) {
      transfer_from_ids_.push_back(transfer_from_id);
      transfer_to_ids_.push_back(transfer_to_id);

      if (max_id_ < transfer_from_id || max_id_ < transfer_to_id) {
        max_id_ = transfer_from_id > transfer_to_id ? transfer_from_id : transfer_to_id;
      }
    }
    ConstructAdjacencyList();
  }

  ~DirectedGraph() {}

  std::vector<std::vector<int> > FindAllCycles(std::vector<std::vector<int> >* ret) {
    // map to represent the status of each vertex
    // 1 represents the vertex has already been visited
    // 0 represents the vertex has not been visited
    // -1 represents the vertex can be skipped(not envovled in the ids or finished processed)
    std::vector<int> status_map(max_id_+1, -1);
    std::vector<bool> appear_in_from(max_id_+1, false);
    std::vector<bool> appear_in_to(max_id_+1, false);
    for (int i = 0; i < transfer_from_ids_.size(); i++) {
      appear_in_from[transfer_from_ids_[i]] = true;
      appear_in_to[transfer_to_ids_[i]] = true;
    }
    for (int i = 0; i < appear_in_from.size(); i++) {
      status_map[i] = appear_in_from[i] && appear_in_to[i] == true ? 0 : -1;
    }

    std::vector<int> path;
    for (int i = 0; i < status_map.size(); i++) {
      if (status_map[i] == -1) continue;
      status_map[i] = 1;
      path.push_back(i);
      DepthFirstSearch(vertexes_[i]->val, &path, &status_map, ret);
      status_map[i] = -1;
      path.pop_back();
    }

    std::vector<std::vector<int> > ret_after_sort;
    for (int i = 0; i < ret->size(); i++) {
      if ((*ret)[i].size() < 3 || (*ret)[i].size() > 7)
        continue;
      RotatePath(&(*ret)[i]);
      ret_after_sort.push_back((*ret)[i]);
    }
    std::sort(ret_after_sort.begin(), ret_after_sort.end(), Compare);

    std::vector<std::vector<int> > ret_final;
    ret_final.push_back(ret_after_sort[0]);
    for (int i = 1; i < ret_after_sort.size(); i++) {
      if (IsSame(&ret_after_sort[i], &ret_after_sort[i-1]))
        continue;
      ret_final.push_back(ret_after_sort[i]);
    }

    return ret_final;
  }

private:
  std::vector<int> transfer_from_ids_;
  std::vector<int> transfer_to_ids_;
  int max_id_;

  std::vector<ListNode*> vertexes_;

  void AddEdge(int src, int dest) {
    ListNode* current = vertexes_[src];
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = new ListNode(dest);
  }

  void ConstructAdjacencyList() {
    for (int i = 0; i <= max_id_; i++) {
      ListNode* vertex = new ListNode(i);
      vertexes_.push_back(vertex);
    }

    for (int i = 0; i < transfer_from_ids_.size(); i++) {
      AddEdge(transfer_from_ids_[i], transfer_to_ids_[i]);
    }
  }

  void DepthFirstSearch(int id, std::vector<int>* path, std::vector<int>* status_map, 
      std::vector<std::vector<int> >* ret) {

    ListNode* current = vertexes_[id]->next;
    while (current != NULL) {
      if ((*status_map)[current->val] == -1) {

      } else if ((*status_map)[current->val] == 1) {
        int i = 0;
        for (; i < path->size(); i++) {
          if (current->val == (*path)[i])
            break; 
        }
        std::vector<int> result; 
        for (int j = i; j < path->size(); j++) {
          result.push_back((*path)[j]);
        }
        ret->push_back(result);
      } else {
        (*status_map)[current->val] = 1;
        path->push_back(current->val);
        DepthFirstSearch(current->val, path, status_map, ret);
        (*status_map)[current->val] = 0;
        path->pop_back();
      }
      current = current->next;
    }
  }

  void RotatePath(std::vector<int>* path) {
    int min_idx = 0;
    for (int i = 0; i < path->size(); i++) {
      min_idx = (*path)[min_idx] < (*path)[i] ? min_idx : i;
    }
    std::vector<int> tmp(*path);
    for (int i = 0; i < path->size(); i++) {
      (*path)[i] = tmp[(i+min_idx)%path->size()];
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

  bool IsSame(std::vector<int>* a, std::vector<int>* b) {
    if (a->size() != b->size())
      return false;
    for (int i = 0; i < a->size(); i++) {
      if ((*a)[i] != (*b)[i])
        return false;
    }
    return true;
  }
};

int main(int argc, char** argv) {
  DirectedGraph directed_graph("../data/test_data.txt");

  std::vector<std::vector<int> > all_cycles;
  std::vector<std::vector<int> > ret;
  ret = directed_graph.FindAllCycles(&all_cycles);

  std::ofstream outfile("go.txt", std::ios::out);
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