#include <cstddef>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

typedef struct ListNode {
  int val;
  ListNode* next;

  ListNode(int val_) : val(val_) {
    next = NULL;
  }
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
};

int main(int argc, char** argv) {
  DirectedGraph("../data/test_data.txt");

  return 0;
}