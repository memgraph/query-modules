// #include <omp.h>

#include <chrono>
#include <numeric>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include "fibonacci_heap.hpp"

#include <mg_exceptions.hpp>
#include <mg_utils.hpp>

namespace {

struct PriorityPathItem {
  std::int32_t distance;
  std::uint64_t vertex;
  std::vector<std::uint64_t> path;

  bool operator<(const PriorityPathItem &other) const { return distance > other.distance; }
};

constexpr char const *kProcedureGet = "get";

constexpr char const *kArgumentSources = "sources";
constexpr char const *kArgumentTargets = "targets";

constexpr char const *kFieldSource = "source";
constexpr char const *kFieldTarget = "target";
constexpr char const *kFieldPath = "path";

void InsertPathResult(mgp_graph *graph, mgp_result *result, mgp_memory *memory, std::uint64_t source_id,
                      std::uint64_t target_id, std::vector<std::uint64_t> &edge_ids, mg_utility::EdgeStore &store) {
  auto *record = mgp::result_new_record(result);

  auto edges_size = edge_ids.size();
  auto path = mgp::path_make_with_start(mgp::edge_get_from(store.Get(edge_ids[0])), memory);
  for (std::int32_t i = 0; i < edges_size; ++i) {
    auto edge = store.Get(edge_ids[i]);
    mgp::path_expand(path, edge);
  }

  // Insert records in Memgraph
  mg_utility::InsertNodeValueResult(graph, record, kFieldSource, source_id, memory);
  mg_utility::InsertNodeValueResult(graph, record, kFieldTarget, target_id, memory);
  mg_utility::InsertPathValueResult(record, kFieldPath, path, memory);
}

std::vector<std::uint64_t> TransformNodeIDs(const mg_graph::GraphView<> &mg_graph,
                                            std::vector<std::uint64_t> &mg_nodes) {
  std::vector<std::uint64_t> nodes;
  nodes.reserve(mg_nodes.size());
  std::transform(
      mg_nodes.begin(), mg_nodes.end(), std::back_inserter(nodes),
      [&mg_graph](const std::uint64_t node_id) -> std::uint64_t { return mg_graph.GetInnerNodeId(node_id); });
  return nodes;
}

std::vector<std::uint64_t> FetchAllNodesIDs(const mg_graph::GraphView<> &mg_graph) {
  std::vector<uint64_t> nodes(mg_graph.Nodes().size());
  std::iota(nodes.begin(), nodes.end(), 0);
  return nodes;
}

std::vector<std::uint64_t> FetchNodeIDs(const mg_graph::GraphView<> &mg_graph, mgp_list *mg_nodes) {
  std::vector<uint64_t> nodes;
  if (mg_nodes != nullptr) {
    auto sources_arg = mg_utility::GetNodeIDs(mg_nodes);
    nodes = TransformNodeIDs(mg_graph, sources_arg);
  } else {
    nodes = FetchAllNodesIDs(mg_graph);
  }
  return nodes;
}

// TODO  pass reference?
void DFS_get_paths(std::unordered_map<std::uint64_t, std::vector<std::pair<std::uint64_t, std::uint64_t>>> &prev,
                   std::uint64_t target_v, std::uint64_t current_v, std::vector<std::uint64_t> &path, mgp_graph *graph,
                   mgp_result *result, mgp_memory *memory, mg_utility::EdgeStore &store) {
  // checking for prev[target] existance is before recursive, later
  // check if target is source
  if (prev[current_v][0].first == -1) {
#pragma omp critical
    InsertPathResult(graph, result, memory, current_v, target_v, path, store);
    return;
  }

  for (std::pair<uint64_t, uint64_t> par : prev[current_v]) {
    // could the push_back and pop_back be placed outside of for?
    path.push_back(par.second);

    DFS_get_paths(prev, target_v, par.first, path, graph, result, memory, store);

    path.pop_back();
  }
}

void ShortestPath(mgp_list *args, mgp_graph *memgraph_graph, mgp_result *result, mgp_memory *memory) {
  try {
    // Fetch the target & source IDs
    auto sources_arg =
        !mgp::value_is_null(mgp::list_at(args, 0)) ? mgp::value_get_list(mgp::list_at(args, 0)) : nullptr;

    auto targets_arg =
        !mgp::value_is_null(mgp::list_at(args, 1)) ? mgp::value_get_list(mgp::list_at(args, 1)) : nullptr;

    auto res = mg_utility::GetGraphViewWithEdge(memgraph_graph, result, memory, mg_graph::GraphType::kDirectedGraph);
    const auto &graph = res.first;
    const auto &edge_store = res.second;

    // Fetch target inner IDs. If not provided, fetch all.
    auto targets_tmp = FetchNodeIDs(*graph.get(), targets_arg);
    std::unordered_set<std::uint64_t> targets(targets_tmp.begin(), targets_tmp.end());

    // Fetch sources inner IDs. If not provided, fetch all.
    auto sources = FetchNodeIDs(*graph.get(), sources_arg);
    auto sources_size = sources.size();

    // Dijsktra with priority queue. Parallel for each source
#pragma omp parallel for
    for (std::size_t i = 0; i < sources_size; ++i) {
      auto source = sources[i];

      // visited? shortest_path_length?

      fibonacci_heap<std::int32_t, std::uint64_t> *priority_queue;
      priority_queue = new fibonacci_heap<std::int32_t, std::uint64_t>([](int k1, int k2) { return k1 < k2; });
      std::unordered_map<std::uint64_t, std::vector<std::pair<std::uint64_t, std::uint64_t>>> prev;
      std::unordered_map<std::uint64_t, std::uint32_t> dist;

      // warning: assinging -1 to uint
      prev[source].push_back(std::make_pair(-1, -1));
      priority_queue->insert(0, source);

      while (!priority_queue->empty()) {
        auto [distance, node_id] = priority_queue->get();
        priority_queue->remove();

        // No expansion if distance is higher - if visited and larger dist - skip -> taken care of?

        // critical section inserting a path in Memgraph result - after traversal
        // provjera if path empty?

        // Traverse in-neighbors and append to priority queue
        for (auto [nxt_vertex_id, nxt_edge_id] : graph.get()->InNeighbours(node_id)) {
          auto nxt_distance = distance + 1;

          if (dist.find(nxt_vertex_id) == dist.end()) {
            // hasn't been visited yet
            dist[nxt_vertex_id] = nxt_distance;
            prev[nxt_vertex_id].push_back(std::make_pair(node_id, nxt_edge_id));
            priority_queue->insert(nxt_distance, nxt_vertex_id);
          } else if (nxt_distance < dist[nxt_vertex_id]) {
            // has been visited, but found a shorter path
            prev[nxt_vertex_id] = std::vector<std::pair<std::uint64_t, std::uint64_t>>();
            prev[nxt_vertex_id].push_back(std::make_pair(node_id, nxt_edge_id));
            dist[nxt_vertex_id] = nxt_distance;
            priority_queue->update_key(nxt_distance, nxt_vertex_id);
          } else if (nxt_distance == dist[nxt_vertex_id]) {
            // found a path of similar length
            prev[nxt_vertex_id].push_back(std::make_pair(node_id, nxt_edge_id));
          }
        }
      }

      DFS_get_paths(source, prev, targets);
    }
  } catch (const std::exception &e) {
    mgp::result_set_error_msg(result, e.what());
    return;
  }
}
}  // namespace

extern "C" int mgp_init_module(struct mgp_module *module, struct mgp_memory *memory) {
  try {
    auto *wcc_proc = mgp::module_add_read_procedure(module, kProcedureGet, ShortestPath);

    auto default_null = mgp::value_make_null(memory);
    mgp::proc_add_opt_arg(wcc_proc, kArgumentSources, mgp::type_nullable(mgp::type_list(mgp::type_node())),
                          default_null);
    mgp::proc_add_opt_arg(wcc_proc, kArgumentTargets, mgp::type_nullable(mgp::type_list(mgp::type_node())),
                          default_null);

    mgp::proc_add_result(wcc_proc, kFieldSource, mgp::type_node());
    mgp::proc_add_result(wcc_proc, kFieldTarget, mgp::type_node());
    mgp::proc_add_result(wcc_proc, kFieldPath, mgp::type_path());

    mgp::value_destroy(default_null);
  } catch (const std::exception &e) {
    return 1;
  }

  return 0;
}

extern "C" int mgp_shutdown_module() { return 0; }
