#include <cstddef>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

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

  void FindAllCycles(std::vector<std::vector<int> >* ret) {
    // map to represent the status of each vertex
    // 1 represents the vertex has already been visited
    // 0 represents the vertex has not been visited
    // -1 represents the vertex can be skipped(not envovled in the ids or finished processed)
    std::vector<int> status_map(max_id_+1, -1);
    for (int i = 0; i < transfer_from_ids_.size(); i++) {
      status_map[transfer_from_ids_[i]] = 0;
    }
    for (int i = 0; i < transfer_to_ids_.size(); i++) {
      status_map[transfer_to_ids_[i]] = 0;
    }

    std::vector<int> path;
    for (int i = 0; i < status_map.size(); i++) {
      if (status_map[i] == -1) continue;
      status_map[i] = 1;
      path.push_back(i);
      DepthFirstSearch(vertexes_[i]->val, &path, &status_map, ret);
      status_map[i] = 0;
      path.pop_back();
    }
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
};

int main(int argc, char** argv) {
  DirectedGraph directed_graph("../data/test_data.txt");

  std::vector<std::vector<int> > ret;
  directed_graph.FindAllCycles(&ret);
  std::cout << ret.size() << std::endl;
  for (int i = 0; i < ret[0].size(); i++) {
    std::cout << ret[1][i] << std::endl;
  }
  

  

  return 0;
}