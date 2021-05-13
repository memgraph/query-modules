#include <queue>
#include <stack>
#include <vector>

#include "betweenness_centrality.hpp"

namespace betweenness_centrality_util {

void BFS(const std::uint64_t source_node, const mg_graph::GraphView<> &graph, std::stack<std::uint64_t> &visited,
         std::vector<std::vector<std::uint64_t>> &predecessors, std::vector<std::uint64_t> &shortest_paths_counter) {
  auto number_of_nodes = graph.Nodes().size();

  // -1 to indicate that node is not visited
  std::vector<int> distance(number_of_nodes, -1);

  shortest_paths_counter[source_node] = 1;
  distance[source_node] = 0;

  std::queue<std::uint64_t> BFS_queue;
  BFS_queue.push(source_node);

  while (!BFS_queue.empty()) {
    auto current_node_id = BFS_queue.front();
    BFS_queue.pop();
    visited.push(current_node_id);

    for (auto neighbor : graph.Neighbours(current_node_id)) {
      auto neighbor_id = neighbor.node_id;

      // node found for the first time
      if (distance[neighbor_id] < 0) {
        BFS_queue.push(neighbor_id);
        distance[neighbor_id] = distance[current_node_id] + 1;
      }

      // shortest path from node to neighbor_id goes through current_node
      if (distance[neighbor_id] == distance[current_node_id] + 1) {
        shortest_paths_counter[neighbor_id] += shortest_paths_counter[current_node_id];
        predecessors[neighbor_id].emplace_back(current_node_id);
      }
    }
  }
}

void Normalize(std::vector<double> &vec, double constant) {
  for (auto index = 0; index < vec.size(); index++) {
    vec[index] *= constant;
  }
}

}  // namespace betweenness_centrality_util

namespace betweenness_centrality_alg {

std::vector<double> BetweennessCentrality(const mg_graph::GraphView<> &graph, bool directed, bool normalized) {
  auto number_of_nodes = graph.Nodes().size();
  std::vector<double> betweenness_centrality(number_of_nodes, 0);

  // perform bfs for every node in the graph
  for (std::uint64_t node_id = 0; node_id < number_of_nodes; node_id++) {
    // data structures used in BFS
    std::stack<std::uint64_t> visited;
    std::vector<std::vector<std::uint64_t>> predecessors(number_of_nodes, std::vector<std::uint64_t>());
    std::vector<std::uint64_t> shortest_paths_counter(number_of_nodes, 0);
    betweenness_centrality_util::BFS(node_id, graph, visited, predecessors, shortest_paths_counter);

    std::vector<double> dependency(number_of_nodes, 0);

    while (!visited.empty()) {
      auto current_node = visited.top();
      visited.pop();

      for (auto p : predecessors[current_node]) {
        double fraction = (double)shortest_paths_counter[p] / shortest_paths_counter[current_node];
        dependency[p] += fraction * (1 + dependency[current_node]);
      }

      if (current_node != node_id) {
        if (directed) betweenness_centrality[current_node] += dependency[current_node];
        // centrality scores need to be divided by two since all shortest paths are considered twice
        else
          betweenness_centrality[current_node] += dependency[current_node] / 2.0;
      }
    }
  }

  if (normalized) {
    // normalized by dividing the value by the number of pairs of nodes not including the node whose value we normalize
    auto number_of_pairs = (number_of_nodes - 1) * (number_of_nodes - 2);
    if (directed) {
      double constant = number_of_nodes > 2 ? static_cast<double>(1) / number_of_pairs : 1.0;
      betweenness_centrality_util::Normalize(betweenness_centrality, constant);
    } else {
      double constant = number_of_nodes > 2 ? static_cast<double>(2) / number_of_pairs : 1.0;
      betweenness_centrality_util::Normalize(betweenness_centrality, constant);
    }
  }

  return betweenness_centrality;
}

}  // namespace betweenness_centrality_alg
