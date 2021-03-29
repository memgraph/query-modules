/// @file graph.hpp
/// This file contains the graph definitions.

#pragma once

#include <algorithm>
#include <map>
#include <unordered_map>

#include "data_structures/graph_data.hpp"
#include "data_structures/graph_view.hpp"
#include "mg_exceptions.hpp"

namespace mg_graph {

/// Graph representation.
template <typename TSize> class Graph : public GraphView<TSize> {

  using TNode = Node<TSize>;
  using TEdge = Edge<TSize>;
  using TNeighbour = Neighbour<TSize>;

  static_assert(
      std::is_same<TSize, uint8_t>::value ||
          std::is_same<TSize, uint16_t>::value ||
          std::is_same<TSize, uint32_t>::value ||
          std::is_same<TSize, uint64_t>::value,
      "mg_graph::Graph expects the type to be an unsigned integer type\n"
      "only (uint8_t, uint16_t, uint32_t, or uint64_t).");

public:
  /// Create object Graph
  explicit Graph() = default;

  /// Destroys the object.
  ~Graph() = default;

  /// Gets all graph nodes.
  ///
  /// @return Vector of nodes
  const std::vector<TNode> &Nodes() const override { return nodes_; }

  /// Gets all graph edges.
  ///
  /// This method always returns a complete list of edges. If some edges are
  /// deleted with EraseEdge method, this method will return also deleted edges.
  /// Deleted edges will have invalid edge id. Method CheckIfEdgeValid is used
  /// for checking the edge. Recommendation is to use EraseEdge method only in
  /// test cases.
  /// @return Vector of edges.
  const std::vector<TEdge> &Edges() const override { return edges_; }

  /// Gets the edges between two neighbour nodes.
  ///
  /// @param[in] first node id
  /// @param[in] second node id
  ///
  /// @return     Iterator range
  std::vector<TSize> GetEdgesBetweenNodes(TSize first,
                                          TSize second) const override {
    std::vector<TSize> ret;
    const auto [range_start, range_end] =
        nodes_to_edge_.equal_range(std::minmax(first, second));
    ret.reserve(std::distance(range_start, range_end));
    for (auto it = range_start; it != range_end; ++it) {
      if (IsEdgeValid(it->second)) {
        ret.push_back(it->second);
      }
    }
    return ret;
  }

  /// Gets all incident edges ids.
  ///
  /// @return all incident edges
  const std::vector<TSize> &IncidentEdges(TSize node_id) const override {
    if (node_id < 0 && node_id >= nodes_.size())
      throw mg_exception::InvalidIDException();
    return adj_list_[node_id];
  }

  /// Gets neighbour nodes.
  ///
  /// @param[in] node_id target node id
  ///
  /// @return vector of neighbours
  const std::vector<TNeighbour> &Neighbours(TSize node_id) const override {
    if (node_id < 0 && node_id >= nodes_.size())
      throw mg_exception::InvalidIDException();
    return neighbours_[node_id];
  }

  /// Gets node with node id.
  ///
  /// @param[in] node_id node id
  ///
  /// @return target Node struct
  const TNode &GetNode(TSize node_id) const override {
    if (node_id < 0 && node_id >= nodes_.size())
      throw mg_exception::InvalidIDException();
    return nodes_[node_id];
  }

  /// Gets Edge with edge id.
  ///
  /// @param[in] edge_id edge id
  ///
  /// @return Edge struct
  const TEdge &GetEdge(TSize edge_id) const override { return edges_[edge_id]; }

  /// Creates a node.
  ///
  /// @return     Created node id
  TSize CreateNode(uint64_t memgraph_id) {
    TSize id = nodes_.size();
    nodes_.push_back({id});
    adj_list_.emplace_back();
    neighbours_.emplace_back();

    inner_to_memgraph_id_.insert(std::pair(id, memgraph_id));
    memgraph_to_inner_id_.insert(std::pair(memgraph_id, id));
    return id;
  }

  /// Creates an edge.
  ///
  /// Creates an undirected edge in the graph, but edge will contain information
  /// about the original directed property.
  ///
  /// @param[in]  from  The from node identifier
  /// @param[in]  to    The to node identifier
  ///
  /// @return     Created edge id
  TSize CreateEdge(uint64_t memgraph_id_from, uint64_t memgraph_id_to) {
    TSize from = memgraph_to_inner_id_[memgraph_id_from];
    TSize to = memgraph_to_inner_id_[memgraph_id_to];
    if (from < 0 || to < 0 || from >= nodes_.size() || to >= nodes_.size()) {
      throw mg_exception::InvalidIDException();
    }
    TSize id = edges_.size();
    edges_.push_back({id, from, to});
    adj_list_[from].push_back(id);
    adj_list_[to].push_back(id);
    neighbours_[from].emplace_back(to, id);
    neighbours_[to].emplace_back(from, id);
    nodes_to_edge_.insert({std::minmax(from, to), id});
    return id;
  }

  /// Gets all valid edges.
  ///
  /// Edge is valid if is not deleted with EraseEdge method.
  ///
  /// @return Vector of valid edges
  std::vector<TEdge> ExistingEdges() const {
    std::vector<TEdge> output;
    output.reserve(edges_.size());
    for (const auto &edge : edges_) {
      if (edge.id == Graph::kDeletedEdgeId)
        continue;
      output.push_back(edge);
    }
    return output;
  }

  /// Checks if edge is valid.
  ///
  /// Edge is valid if is created and if is not deleted.
  ///
  /// @return true if edge is valid, otherwise returns false
  bool IsEdgeValid(TSize edge_id) const {
    if (edge_id < 0 || edge_id >= edges_.size())
      return false;
    if (edges_[edge_id].id == kDeletedEdgeId)
      return false;
    return true;
  }

  /// Removes edge from graph.
  ///
  /// Recommendation is to use this method only in the tests.
  ///
  /// @param[in] u node id of node on same edge
  /// @param[in] v node id of node on same edge
  void EraseEdge(TSize u, TSize v) {
    if (u < 0 && u >= nodes_.size())
      throw mg_exception::InvalidIDException();
    if (v < 0 && v >= nodes_.size())
      throw mg_exception::InvalidIDException();

    auto it = nodes_to_edge_.find(std::minmax(u, v));
    if (it == nodes_to_edge_.end())
      return;
    TSize edge_id = it->second;

    for (auto it = adj_list_[u].begin(); it != adj_list_[u].end(); ++it) {
      if (edges_[*it].to == v || edges_[*it].from == v) {
        edges_[*it].id = Graph::kDeletedEdgeId;
        adj_list_[u].erase(it);
        break;
      }
    }
    for (auto it = adj_list_[v].begin(); it != adj_list_[v].end(); ++it) {
      if (edges_[*it].to == u || edges_[*it].from == u) {
        edges_[*it].id = Graph::kDeletedEdgeId;
        adj_list_[v].erase(it);
        break;
      }
    }

    for (auto it = neighbours_[u].begin(); it != neighbours_[u].end(); ++it) {
      if (it->edge_id == edge_id) {
        neighbours_[u].erase(it);
        break;
      }
    }
    for (auto it = neighbours_[v].begin(); it != neighbours_[v].end(); ++it) {
      if (it->edge_id == edge_id) {
        neighbours_[v].erase(it);
        break;
      }
    }
  }

  ///
  /// Returns the Memgraph database ID from graph view
  ///
  /// @param node_id view's inner ID
  ///
  TSize GetMemgraphNodeId(uint64_t node_id) {
    if (inner_to_memgraph_id_.find(node_id) == inner_to_memgraph_id_.end()) {
      throw mg_exception::InvalidIDException();
    }
    return inner_to_memgraph_id_[node_id];
  }

  /// Removes all edges and nodes from graph.
  void Clear() {
    adj_list_.clear();
    nodes_.clear();
    edges_.clear();
    nodes_to_edge_.clear();
    neighbours_.clear();

    memgraph_to_inner_id_.clear();
    inner_to_memgraph_id_.clear();
  }

private:
  // Constant is used for marking deleted edges.
  // If edge id is equal to constant, edge is deleted.
  static const TSize kDeletedEdgeId = std::numeric_limits<TSize>::max();

  std::vector<std::vector<TSize>> adj_list_;
  std::vector<std::vector<TNeighbour>> neighbours_;

  std::vector<TNode> nodes_;
  std::vector<TEdge> edges_;
  std::unordered_map<TSize, uint64_t> inner_to_memgraph_id_;
  std::unordered_map<uint64_t, TSize> memgraph_to_inner_id_;

  std::multimap<std::pair<TSize, TSize>, TSize> nodes_to_edge_;
};
} // namespace mg_graph
