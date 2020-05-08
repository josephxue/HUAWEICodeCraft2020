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
    delete[](ids_comma_); delete[](ids_line_); delete[](sl_);
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

    ids_comma_ = new int8_t[ids_num_*16];
    ids_line_  = new int8_t[ids_num_*16];
    sl_= new int[ids_num_];

    unsigned int id, t;
    for (int i = 0; i < ids_num_; i++) {
      id = ids[i];
      idHash[id] = i;
      t = sprintf((char*)ids_comma_+i*16, "%u,",  id);
      sprintf((char*)ids_line_+i*16, "%u\n", id);
      sl_[i] = t;
    }

    G = vector<vector<int>>(ids_num_);
    inDegrees = vector<int>(ids_num_, 0);

    int u, v;
    for (int i = 0; i < input_ids_size_; i += 2) {
      u = idHash[input_ids_[i]]; v = idHash[input_ids_[i+1]];
      G[u].push_back(v);
      ++inDegrees[v];
    }
    for (auto& g : G) {
      std::sort(g.begin(), g.end());
    }
  }

  //magic code,don't touch
  bool checkAns(int (&path)[7], int depth) {
    for (int i = 0; i < depth; i++) {
      int l = ids[path[(i+depth-1)%depth]], m = ids[path[i]], r = ids[path[(i+1)%depth]];
      if (!check(migic[(unsigned long long)ids[path[(i+depth-1)%depth]] << 32 | ids[path[i]]], 
                 migic[(unsigned long long) ids[path[i]] << 32 | ids[path[(i+1)%depth]]])) return false;
    }
    return true;
  }

  void dfs(int head, int cur, int depth, int (&path)[7]) {
    vis[cur] = true;
    path[depth-1] = cur;
    for (int &v:G[cur]) {
      if (v == head && depth >= 3 && depth <= 7 && checkAns(path, depth)) {
        for (int i = 0; i < depth-1; i++) {
          tmpret = vld1q_s8(ids_comma_+path[i]*16); 
          vst1q_s8(tmps[depth-3], tmpret); 
          s = sl_[path[i]]; 
          ret_num_[depth-3] += s; 
          tmps[depth-3] += s;
        }
        tmpret = vld1q_s8(ids_line_+path[depth-1]*16); 
        vst1q_s8(tmps[depth-3], tmpret); 
        s = sl_[path[depth-1]]; 
        ret_num_[depth-3] += s; 
        tmps[depth-3] += s;
        path_num_++;
      }
      if (depth < 7 && !vis[v] && v > head) {
        dfs(head, v, depth+1, path);
      }
    }
    vis[cur] = false;
  }

  void solve() {
    vis = vector<bool>(ids_num_, false);
    int path[7];
    for (int i = 0; i < ids_num_; i++) {
      if (!G[i].empty()) {
        dfs(i, i, 1, path);
      }
    }
  }

  void save(string &output_filename) {
    FILE *fp = fopen(output_filename.c_str(), "wb");
    if (fp == NULL) {
      printf("file open error\n");
      exit(1);
    }
    
    char buf[15];
    sprintf(buf, "%d\n", path_num_);
    fwrite(buf, strlen(buf), sizeof(char), fp);

    for (int i = 0; i < 5; i++)
      fwrite((char*)ret_[i], ret_num_[i], sizeof(char), fp);

    fclose(fp);
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
  int8_t* ids_comma_;
  int8_t* ids_line_;
  int* sl_;

  int8_t* ret3_ = new int8_t[33*20000000]; 
  int8_t* ret4_ = new int8_t[44*20000000]; 
  int8_t* ret5_ = new int8_t[55*20000000];
  int8_t* ret6_ = new int8_t[66*20000000];
  int8_t* ret7_ = new int8_t[77*20000000];
  int8_t* ret_[5] = {ret3_, ret4_, ret5_, ret6_, ret7_};
  int ret_num_[5] = {0, 0, 0, 0, 0}; 
  int path_num_ = 0;

  int8x16_t tmpret;
  int8_t* s3 = ret3_;
  int8_t* s4 = ret4_;
  int8_t* s5 = ret5_;
  int8_t* s6 = ret6_;
  int8_t* s7 = ret7_;
  int8_t* tmps[5] = {s3, s4, s5, s6, s7};
  int s;
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
