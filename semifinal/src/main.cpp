#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>    
#include <sys/stat.h>   
#include <sys/mman.h>
#include <fcntl.h>
#include <arm_neon.h>

#include <queue>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <unordered_map>


using namespace std;


class Solution {
public:
  ~Solution() {
    delete[](ids);
    delete[](input_ids_);
  }

  inline bool check(int x, int y) {
      if(x==0 || y==0) return false;
      return x <= 5ll * y && y <= 3ll * x;
  }

  void parseInput(std::string& test_filename) {
    int fd = open(test_filename.c_str(), O_RDONLY);
    if (fd == -1) {
      printf("file open error\n");
      exit(1);
    }

    struct stat st;
    int r = fstat(fd, &st);
    if (r == -1) {
      printf("get file stat error\n");
      close(fd);
      exit(-1);
    }
    int filesize = st.st_size;
  
    int8_t* p;
    int8_t* start; int8_t* current;
    int8_t  buf[16];

    p = (int8_t*)mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, 0);
    if (p == NULL || p == (void*)(-1)) {
      printf("mmap error\n");
      close(fd);
      exit(-1);
    }
  
    start = p; current = p; 
    int s;
    int split = 0;
    int8x16_t tmp;
    unsigned int val1, val2;
    for (int i = 0; i < filesize; i++, current++) {
      if (*current == '\n' || *current == ',') {
        s = current - start;
        tmp = vld1q_s8(start);
        vst1q_s8(buf, tmp);
        buf[s] = '\0';

        switch(++split%3) {
          case 1: {
            val1 = strtoul((char*)buf, NULL, 10);
            start = current+1;
            break;
          }
          case 2: {
            val2 = strtoul((char*)buf, NULL, 10);
            start = current+1;
            input_ids_[input_ids_size_++] = val1;
            input_ids_[input_ids_size_++] = val2;
            break;
          }
          case 0: {
            migic[(unsigned long long) val1 << 32 | val2] = strtoul((char*)buf, NULL, 10);
            start = current+1;
            break;
          }
        }
      }
    }

    munmap((void*)p, filesize);
    close(fd);
  }

  void constructGraph() {
    ids = new unsigned int[input_ids_size_];
    memcpy(ids, input_ids_, sizeof(unsigned int)*input_ids_size_);
    std::sort(ids, ids+input_ids_size_);
    ids_num_ = std::unique(ids, ids+input_ids_size_) - ids;

    for (int i = 0; i < ids_num_; i++) {
      idHash[ids[i]] = i;
    }

    G = vector<vector<int>>(ids_num_);
    inDegrees = vector<int>(ids_num_, 0);
    for (int i = 0; i < input_ids_size_; i += 2) {
      int u = idHash[input_ids_[i]], v = idHash[input_ids_[i+1]];
      G[u].push_back(v);
      ++inDegrees[v];
    }
    for (auto& g : G) {
      std::sort(g.begin(), g.end());
    }
  }

  //magic code,don't touch
  bool checkAns(vector<unsigned int> tmp, int depth) {
      for (int i = 0; i < depth; i++) {
          int l = tmp[(i + depth - 1) % depth], m = tmp[i], r = tmp[(i + 1) % depth];
          if (!check(migic[(unsigned long long) l << 32 | m], migic[(unsigned long long) m << 32 | r])) return false;
      }
      return true;
  }

  void dfs(int head, int cur, int depth, int (&path)[7]) {
    vis[cur] = true;
    path[depth-1] = cur;
    for (int &v:G[cur]) {
      if (v == head && depth >= 3 && depth <= 7) {
        vector<unsigned int> tmp;
        for (int i = 0; i < depth; i++) {
          tmp.push_back(ids[path[i]]);
        }
        if (checkAns(tmp, depth)) {
          ret_[depth-3].emplace_back(tmp);
        }
      }
      if (depth < 7 && !vis[v] && v > head) {
        dfs(head, v, depth+1, path);
      }
    }
    vis[cur] = false;
  }

  void solve() {
    ret_.resize(5);
    vis = vector<bool>(ids_num_, false);
    int path[7];
    for (int i = 0; i < ids_num_; i++) {
      if (!G[i].empty()) {
        dfs(i, i, 1, path);
      }
    }
  }

  void save(string &outputFile) {
    std::ofstream out(outputFile);
    out << ret_[0].size()+ret_[1].size()+ret_[2].size()+ret_[3].size()+ret_[4].size() << endl;
    for (int i = 0; i < ret_.size(); i++) {
      for (auto& path : ret_[i]) {
        out << path[0];
        for (int j = 1; j < path.size(); j++) {
          out << "," << path[j];
        }
        out << std::endl;
      }
    }
  }

private:
  vector<vector<int>> G;
  unordered_map<unsigned int, int> idHash; //sorted id to 0...n
  //bad case, don't follow
  unordered_map<unsigned long long, int> migic;
  unsigned int* ids;
  unsigned int* input_ids_ = new unsigned int[4000000]; int input_ids_size_ = 0;
  vector<int> inDegrees;
  vector<bool> vis;
  int ids_num_;

  std::vector<std::vector<std::vector<unsigned int>>> ret_;
  // std::vector<std::string> ret3_;
  // std::vector<std::string> ret4_;
  // std::vector<std::string> ret5_;
  // std::vector<std::string> ret6_;
  // std::vector<std::string> ret7_;
};


void Solve(std::string& test_filename, std::string& output_filename) {
  Solution solution;
  solution.parseInput(test_filename);
  solution.constructGraph();
  solution.solve();
  solution.save(output_filename);
}


int main() {
  // std::string test_filename = "../data/test_data.txt";
  // std::string test_filename = "/root/dataset/28W/test_data28W.txt";
  // std::string test_filename = "/root/dataset/200W/test_data_N111314_E200W_A19630345.txt";
  std::string test_filename = "/data/test_data.txt";

  // std::string output_filename = "result.txt";
  std::string output_filename = "/projects/student/result.txt";

  Solve(test_filename, output_filename);

  return 0;
}
