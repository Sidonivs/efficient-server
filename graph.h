#ifndef __graph_h__
#define __graph_h__

#include <pthread.h>
#include <vector>
#include <math.h>
//#include <unordered_map>
//#include <limits>
#include <queue>
#include "gen/schema.pb.h"

using namespace std;

struct Node {
    int x;
    int y;
    uint32_t id;
    Node *left, *right;
};

struct Dist {
    uint32_t adj_node;
    uint32_t avg_dist;
    uint64_t sum;
    uint32_t counter;
};

class Graph {
  public:
    Graph();
    ~Graph();

    void add_walks(vector<esw::server::Walk>& walks);
    uint64_t find_shortest_path_length(Node& origin, Node& destination);
    uint64_t find_total_length(Node& origin);
    void reset();

  private:
    vector<uint64_t> dijkstra(uint32_t source);
    Node* insert(Node *root, int x, int y, uint32_t depth);
    Node* find_NN(Node *root, int x, int y, uint32_t depth);

    Node *root;
    vector<Node> nodes;
    vector<vector<Dist>> distances;
    //unordered_map<Node, uint32_t> node_to_id_map;
    pthread_rwlock_t lock;
};

#endif