#include "graph.h"

#  define rd_lock(lock) pthread_rwlock_rdlock(lock)
#  define wr_lock(lock) pthread_rwlock_wrlock(lock)
#  define unlock(lock) pthread_rwlock_unlock(lock)

Graph::Graph() {
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    pthread_rwlock_init(&lock, &attr);

    nodes.reserve(100);
    distances.reserve(100);
}

Graph::~Graph() {
    pthread_rwlock_destroy(&lock);
}

void Graph::add_walks(vector<esw::server::Walk>& walks) {
    for (uint32_t i = 0; i < walks.size(); ++i) {
        esw::server::Walk const& walk = walks[i];
        vector<uint32_t> new_node_ids;
    
        rd_lock(&lock);
        uint32_t new_node_id = nodes.size();
        for (uint32_t j = 0; j < walk.locations_size(); ++j) {
            new_node_ids.push_back(new_node_id);
            new_node_id++;

            int x = walk.locations(j).x();
            int y = walk.locations(j).y();

            float euc_dist;
            for (uint32_t k = 0; k < nodes.size(); ++k) {
                euc_dist = sqrt(pow(nodes[k].x - x, 2) + pow(nodes[k].y - y, 2) * 1.0);
                if (euc_dist <= 500) {
                    // locations(j) == nodes[k], maybe update coords?
                    new_node_ids[j] = k;
                    new_node_id--;
                    break;
                }
            }
        }
        unlock(&lock);
        wr_lock(&lock);
        for (uint32_t j = 0; j < walk.locations_size(); ++j) {
            if (new_node_ids[j] < nodes.size()) {
                if (j + 1 != walk.locations_size()) {
                    bool found = false;
                    for (int l = 0; l < distances[new_node_ids[j]].size(); ++l) {
                        if (distances[new_node_ids[j]][l].adj_node == new_node_ids[j+1]) {
                            distances[new_node_ids[j]][l].counter++;
                            distances[new_node_ids[j]][l].sum += walk.lengths(j);
                            distances[new_node_ids[j]][l].avg_dist = distances[new_node_ids[j]][l].sum / distances[new_node_ids[j]][l].counter;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        distances[new_node_ids[j]].push_back({new_node_ids[j+1], walk.lengths(j), walk.lengths(j), 1});
                    }
                }
            } else {
                Node new_node = {walk.locations(j).x(), walk.locations(j).y()};
                nodes.push_back(new_node);
                //node_to_id_map[new_node] = nodes.size() - 1;
                vector<Dist> new_edges;
                if (j + 1 != walk.locations_size()) {
                    new_edges.push_back({new_node_ids[j+1], walk.lengths(j), walk.lengths(j), 1});
                }
                distances.push_back(new_edges);
            }
        }
        unlock(&lock);
    }
}

uint64_t Graph::find_shortest_path_length(Node& origin, Node& destination) {
    /*
    if (auto it_dest = node_to_id_map.find(destination); it_dest != node_to_id_map.end()) {
        if (auto it_orig = node_to_id_map.find(destination); it_orig != node_to_id_map.end()) {
            return dijkstra(it_orig->second)[it_dest->second];
        }
    }
    */

    bool orig_found = false;
    uint32_t orig_id;
    bool dest_found = false;
    uint32_t dest_id;
    float euc_dist;
    rd_lock(&lock);
    for (uint32_t k = 0; k < nodes.size(); ++k) {
        euc_dist = sqrt(pow(nodes[k].x - origin.x, 2) + pow(nodes[k].y - origin.y, 2) * 1.0);
        if (euc_dist <= 500) {
            orig_found = true;
            orig_id = k;
        }

        euc_dist = sqrt(pow(nodes[k].x - destination.x, 2) + pow(nodes[k].y - destination.y, 2) * 1.0);
        if (euc_dist <= 500) {
            dest_found = true;
            dest_id = k;
        }

        if (orig_found && dest_found) {
            unlock(&lock);
            return dijkstra(orig_id)[dest_id];
        }
    }
    unlock(&lock);
    
    cout << "ERROR NOT FOUND ONE TO ONE" << endl;
    return 0;
}

uint64_t Graph::find_total_length(Node& origin) {
    /*
    if (auto it = node_to_id_map.find(origin); it != node_to_id_map.end()) {
        uint64_t sum = 0;
        for (auto& n : dijkstra(it->second)) {
            sum += n;
        }
        return sum;
    }
    */

    float euc_dist;
    rd_lock(&lock);
    for (uint32_t k = 0; k < nodes.size(); ++k) {
        euc_dist = sqrt(pow(nodes[k].x - origin.x, 2) + pow(nodes[k].y - origin.y, 2) * 1.0);
        if (euc_dist <= 500) {
            unlock(&lock);
            uint64_t sum = 0;
            for (auto n : dijkstra(k)) {
                if (n != UINT32_MAX) {
                    sum += n;
                }
            }
            return sum;
        }
    }
    unlock(&lock);
    
    cout << "ERROR NOT FOUND ONE TO ALL" << endl;
    return 0;
}

vector<uint64_t> Graph::dijkstra(uint32_t source) {
    rd_lock(&lock);
    uint32_t num_of_nodes = nodes.size();
    unlock(&lock);
    vector<uint64_t> dist(num_of_nodes, UINT32_MAX);
    dist[source] = 0;

    vector<uint32_t> nums_of_neighbours(num_of_nodes);

    priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<std::pair<int, int>>> prio_queue;

    for (int i = 0; i < num_of_nodes; ++i) {
        prio_queue.push({dist[i], i});
        rd_lock(&lock);
        nums_of_neighbours[i] = distances[i].size();
        unlock(&lock);
    }

    while (!prio_queue.empty()) {
        uint32_t u = prio_queue.top().second;

        for (int j = 0; j < nums_of_neighbours[u]; ++j) {

            rd_lock(&lock);
            uint64_t alt = dist[u] + distances[u][j].avg_dist;
            uint32_t v = distances[u][j].adj_node;
            unlock(&lock);

            if (alt < dist[v]) {
                dist[v] = alt;
                prio_queue.push({alt, v});
            }
        }

        prio_queue.pop();
    }
    /*
    for (auto d : dist) {
        cout << d << endl;
    }
    */
    return dist;
}

void Graph::reset() {
    wr_lock(&lock);
    nodes.clear();
    distances.clear();
    //node_to_id_map.clear();
    unlock(&lock);
}