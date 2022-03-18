#include <algorithm>
#include <memory>
#include <queue>
#include <set>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include <omp.h>

namespace online_bc {

enum class Operation { CREATE_EDGE, CREATE_NODE, CREATE_ATTACH_NODE, DELETE_EDGE, DELETE_NODE, DETACH_DELETE_NODE };

std::unordered_set<std::uint64_t> NeighborsMemgraphIDs(const mg_graph::GraphView<> &graph,
                                                       const std::vector<mg_graph::Neighbour<uint64_t>> neighbors);

class OnlineBC {
 private:
  /// Maps nodes (Memgraph IDs) to their betweenness centrality scores
  std::unordered_map<std::uint64_t, double> node_bc_scores;

  /// Betweenness centrality score initialization flag
  bool initialized = false;

  /// Avoids counting edges twice in case of undirected graphs
  static constexpr double NO_DOUBLE_COUNT = 2.0;

  ///@brief Returns whether previously calculated betweenness centrality scores are inconsistent with the current graph,
  /// i.e. if the nodes which have associated betweenness centrality scores are not the same as the graph’s nodes.
  ///
  ///@param graph Current graph
  bool Inconsistent(const mg_graph::GraphView<> &graph);

  ///@brief Normalize each node’s betweenness centrality score by the number of node pairs not
  /// containing said node. The normalization constant is 2/((N-1)(N-2)) for undirected and 1/((N-1)(N-2)) for directed
  /// graphs, with N being the number of graph nodes.
  ///
  ///@param node_bc_scores Graph nodes with associated betweenness centrality scores
  ///@param graph_order Order (Nº of nodes) of the graph
  ///
  ///@return Normalized betweenness centrality scores
  std::unordered_map<std::uint64_t, double> NormalizeBC(const std::unordered_map<std::uint64_t, double> &node_bc_scores,
                                                        const std::uint64_t graph_order);

  ///@brief Wrapper for the offline betweenness centrality algorithm that maps betweenness centrality scores to their
  /// nodes’ Memgraph IDs.
  ///
  ///@param graph Current graph
  ///@param threads Number of concurrent threads
  void CallBrandesAlgorithm(const mg_graph::GraphView<> &graph, const std::uint64_t threads);

  ///@brief Returns the nodes and the articulation points of the biconnected component affected by the update.
  ///
  ///@param graph Graph containing affected edge (new graph if edge was created, old graph if edge was deleted)
  ///@param updated_edge Created/deleted edge
  ///
  ///@return (nodes, articulation points) in the affected biconnected component
  std::tuple<std::unordered_set<std::uint64_t>, std::unordered_set<std::uint64_t>> IsolateAffectedBCC(
      const mg_graph::GraphView<> &graph, const std::pair<std::uint64_t, std::uint64_t> updated_edge) const;

  ///@brief For each articulation point of the biconnected component affected by the update, returns the order (Nº
  /// of nodes) of the portion of the graph reachable from the articulation point through edges outside that component.
  /// The graph is traversed using a breadth-first search.
  ///
  ///@param graph Graph traversed by the algorithm
  ///@param affected_bcc_articulation_points Articulation points in the affected biconnected component
  ///@param affected_bcc_nodes Nodes in the affected biconnected component
  ///
  ///@return {articulation point ID, attached subgraph order} pairs
  std::unordered_map<std::uint64_t, int> PeripheralSubgraphsOrder(
      const mg_graph::GraphView<> &graph, std::unordered_set<std::uint64_t> affected_bcc_articulation_points,
      std::unordered_set<std::uint64_t> affected_bcc_nodes) const;

  ///@brief Computes lengths of shortest paths between a given node and all other nodes in the graph, restricted to
  /// one biconnected component.
  ///
  ///@param graph Graph traversed by the algorithm
  ///@param graph Source node ID
  ///@param bcc_nodes Nodes in the biconnected component
  ///
  ///@return {node ID, source->node distance} pairs
  // TODO move back to private
  std::unordered_map<std::uint64_t, int> SSSPLengths(const mg_graph::GraphView<> &graph,
                                                     const std::uint64_t source_node_id,
                                                     const std::unordered_set<std::uint64_t> &bcc_nodes) const;

  ///@brief Performs a breadth-first search from a given node akin to the one used by the Brandes’ algorithm. The search
  /// can be restricted to one biconnected component.
  ///
  ///@param graph Graph traversed by the breadth-first search
  ///@param source_node_id Source node ID
  ///@param restrict If true, restricts the search to the affected biconnected component
  ///@param compensate_for_deleted_node If true, guarantees correct results for the DETACH_DELETE_NODE operation
  ///@param affected_bcc_nodes Nodes in the affected biconnected component
  ///
  ///@return {node ID, Nº of shortest paths from the source node} pairs,
  /// {node ID, IDs of immediate predecessors on the shortest paths from the source node} pairs
  /// IDs of nodes visited in the breadth-first search, in reverse order
  std::tuple<std::unordered_map<std::uint64_t, int>, std::unordered_map<std::uint64_t, std::set<std::uint64_t>>,
             std::vector<std::uint64_t>>
  BrandesBFS(const mg_graph::GraphView<> &graph, const std::uint64_t source_node_id, const bool restrict = false,
             const bool compensate_for_deleted_node = false,
             const std::unordered_set<std::uint64_t> &bcc_nodes = {}) const;

  ///@brief Performs an iteration of iCentral that updates the betweenness centrality scores in two steps:
  /// 1) subtracts given node’s old graph contribution to other nodes’ betweenness centrality scores,
  /// 2) adds given node’s new graph contribution to other nodes’ betweenness centrality scores.
  ///
  ///@param prior_graph Graph as before the last update
  ///@param current_graph Current graph
  ///@param s_id ID of the node whose contribution to other nodes’ betweenness centrality scores has changed
  ///@param affected_bcc_nodes Nodes in the affected biconnected component
  ///@param affected_bcc_articulation_points Articulation points in the affected biconnected component
  ///@param peripheral_subgraphs_order {node ID, order of subgraph reachable from node} pairs
  void iCentralIteration(const mg_graph::GraphView<> &prior_graph, const mg_graph::GraphView<> &current_graph,
                         const std::uint64_t s_id, const std::unordered_set<std::uint64_t> &affected_bcc_nodes,
                         const std::unordered_set<std::uint64_t> &affected_bcc_articulation_points,
                         const std::unordered_map<std::uint64_t, int> &peripheral_subgraphs_order);

 public:
  OnlineBC() = default;

  ///@brief Getter for checking initialization status
  ///
  inline bool Initialized() const { return this->initialized; };

  ///@brief Computes initial betweennness centrality scores with Brandes’ algorithm.
  ///
  ///@param graph Current graph
  ///@param normalize If true, normalizes each node’s betweenness centrality score by the number of node pairs not
  /// containing said node. The normalization constant is 2/((N-1)(N-2)) for undirected and 1/((N-1)(N-2)) for directed
  /// graphs, with N being the number of graph nodes.
  ///@param threads Number of concurrent threads
  ///
  ///@return {node ID, BC score} pairs
  std::unordered_map<std::uint64_t, double> Set(const mg_graph::GraphView<> &graph, const bool normalize = true,
                                                const std::uint64_t threads = std::thread::hardware_concurrency());

  ///@brief Returns previously computed betweennness centrality scores.
  /// If this->computed flag is set to false, computes betweennness centrality scores with default parameter values.
  ///
  ///@param graph Current graph
  ///@param normalize If true, normalizes each node’s betweenness centrality score by the number of node pairs not
  /// containing said node. The normalization constant is 2/((N-1)(N-2)) for undirected and 1/((N-1)(N-2)) for directed
  /// graphs, with N being the number of graph nodes.
  ///
  ///@return {node ID, BC score} pairs
  std::unordered_map<std::uint64_t, double> Get(const mg_graph::GraphView<> &graph, const bool normalize = true);

  ///@brief Uses iCentral to recompute betweennness centrality scores after edge updates.
  ///
  ///@param prior_graph Graph as before the last update
  ///@param current_graph Current graph
  ///@param operation Type of graph update (one of {CREATE_EDGE, DELETE_EDGE})
  ///@param updated_edge Created/deleted edge
  ///@param normalize If true, normalizes each node’s betweenness centrality score by the number of node pairs not
  /// containing said node. The normalization constant is 2/((N-1)(N-2)) for undirected and 1/((N-1)(N-2)) for directed
  /// graphs, with N being the number of graph nodes.
  ///@param threads Number of concurrent threads
  ///
  ///@return {node ID, BC score} pairs
  std::unordered_map<std::uint64_t, double> EdgeUpdate(
      const mg_graph::GraphView<> &prior_graph, const mg_graph::GraphView<> &current_graph, const Operation operation,
      const std::pair<std::uint64_t, std::uint64_t> updated_edge, const bool normalize = true,
      const std::uint64_t threads = std::thread::hardware_concurrency());

  ///@brief Uses a single iteration of Brandes’ algorithm to recompute betweennness centrality scores after updates
  /// consisting of an edge and a node solely connected to it.
  ///
  ///@param current_graph Current graph
  ///@param operation Type of graph update (one of {CREATE_ATTACH_NODE, DETACH_DELETE_NODE})
  ///@param updated_node Created/deleted node
  ///@param updated_edge Created/deleted edge
  ///@param normalize If true, normalizes each node’s betweenness centrality score by the number of node pairs not
  /// containing said node. The normalization constant is 2/((N-1)(N-2)) for undirected and 1/((N-1)(N-2)) for directed
  /// graphs, with N being the number of graph nodes.
  ///
  ///@return {node ID, BC score} pairs
  std::unordered_map<std::uint64_t, double> NodeEdgeUpdate(const mg_graph::GraphView<> &current_graph,
                                                           const Operation operation, const std::uint64_t updated_node,
                                                           const std::pair<std::uint64_t, std::uint64_t> updated_edge,
                                                           const bool normalize = true);

  ///@brief Adds or removes the betweenness centrality score entry of created/deleted nodes with degree 0.
  ///
  ///@param operation Type of graph update (one of {CREATE_NODE, DELETE_NODE})
  ///@param updated_node Created/deleted node
  ///@param normalize If true, normalizes each node’s betweenness centrality score by the number of node pairs not
  /// containing said node. The normalization constant is 2/((N-1)(N-2)) for undirected and 1/((N-1)(N-2)) for directed
  /// graphs, with N being the number of graph nodes.
  ///
  ///@return {node ID, BC score} pairs
  std::unordered_map<std::uint64_t, double> NodeUpdate(const Operation operation, const std::uint64_t updated_node,
                                                       const bool normalize = true);
};
}  // namespace online_bc
