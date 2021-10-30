#include <mg_graph.hpp>
#include <mg_utils.hpp>

#include "algorithm/dynamic_community_detection.hpp"

constexpr char const *kFieldNode = "node";
constexpr char const *kFieldCommunityId = "community_id";

constexpr char const *kDirected = "directed";
constexpr char const *kWeighted = "weighted";
constexpr char const *kSimilarityThreshold = "similarity_threshold";
constexpr char const *kExponent = "exponent";
constexpr char const *kMinValue = "min_value";
constexpr char const *kWeightProperty = "weight_property";
constexpr char const *kWSelfloop = "w_selfloop";
constexpr char const *kMaxIterations = "max_iterations";
constexpr char const *kMaxUpdates = "max_updates";

constexpr char const *kFieldMessage = "message";

constexpr char const *kCreatedVertices = "createdVertices";
constexpr char const *kCreatedEdges = "createdEdges";
constexpr char const *kUpdatedVertices = "updatedVertices";
constexpr char const *kUpdatedEdges = "updatedEdges";
constexpr char const *kDeletedVertices = "deletedVertices";
constexpr char const *kDeletedEdges = "deletedEdges";

LabelRankT::LabelRankT algorithm = LabelRankT::LabelRankT();
bool initialized = false;

auto saved_directedness = false;
auto saved_weightedness = false;
std::string saved_weight_property = "weight";
double saved_default_weight = 1;

void InsertCommunityDetectionRecord(mgp_graph *graph, mgp_result *result,
                                    mgp_memory *memory,
                                    const std::uint64_t node_id,
                                    std::uint64_t community_id) {
  auto *record = mgp::result_new_record(result);

  mg_utility::InsertNodeValueResult(graph, record, kFieldNode, node_id, memory);
  mg_utility::InsertIntValueResult(record, kFieldCommunityId, community_id,
                                   memory);
}

void InsertMessageRecord(mgp_result *result, mgp_memory *memory,
                         const char *message) {
  auto *record = mgp::result_new_record(result);

  mg_utility::InsertStringValueResult(record, kFieldMessage, message, memory);
}

void Set(mgp_list *args, mgp_graph *memgraph_graph, mgp_result *result,
         mgp_memory *memory) {
  try {
    auto directed = mgp::value_get_bool(mgp::list_at(args, 0));
    auto weighted = mgp::value_get_bool(mgp::list_at(args, 1));
    auto similarity_threshold = mgp::value_get_double(mgp::list_at(args, 2));
    auto exponent = mgp::value_get_double(mgp::list_at(args, 3));
    auto min_value = mgp::value_get_double(mgp::list_at(args, 4));
    auto weight_property = mgp::value_get_string(mgp::list_at(args, 5));
    auto w_selfloop = mgp::value_get_double(mgp::list_at(args, 6));
    auto max_iterations = mgp::value_get_int(mgp::list_at(args, 7));
    auto max_updates = mgp::value_get_int(mgp::list_at(args, 8));

    ::saved_directedness = directed;
    ::saved_weightedness = weighted;
    ::saved_weight_property = weight_property;

    auto graph_type = saved_directedness
                          ? mg_graph::GraphType::kDirectedGraph
                          : mg_graph::GraphType::kUndirectedGraph;
    auto graph = saved_weightedness
                     ? mg_utility::GetWeightedGraphView(
                           memgraph_graph, result, memory, graph_type,
                           saved_weight_property.c_str(), saved_default_weight)
                     : mg_utility::GetGraphView(memgraph_graph, result, memory,
                                                graph_type);

    auto labels = algorithm.SetLabels(
        graph, directed, weighted, similarity_threshold, exponent, min_value,
        weight_property, w_selfloop, max_iterations, max_updates);
    ::initialized = true;

    for (const auto [node_id, label] : labels) {
      InsertCommunityDetectionRecord(memgraph_graph, result, memory, node_id,
                                     label);
    }
  } catch (const std::exception &e) {
    mgp::result_set_error_msg(result, e.what());
    return;
  }
}

void Get(mgp_list *args, mgp_graph *memgraph_graph, mgp_result *result,
         mgp_memory *memory) {
  try {
    auto graph_type = saved_directedness
                          ? mg_graph::GraphType::kDirectedGraph
                          : mg_graph::GraphType::kUndirectedGraph;
    auto graph = saved_weightedness
                     ? mg_utility::GetWeightedGraphView(
                           memgraph_graph, result, memory, graph_type,
                           saved_weight_property.c_str(), saved_default_weight)
                     : mg_utility::GetGraphView(memgraph_graph, result, memory,
                                                graph_type);

    auto labels =
        initialized ? algorithm.GetLabels(graph) : algorithm.SetLabels(graph);

    for (const auto [node_id, label] : labels) {
      // Previously calculated labels returned by GetLabels() may contain
      // deleted nodes; skip them as they cannot be inserted
      auto *node = mgp::graph_get_vertex_by_id(
          memgraph_graph, mgp_vertex_id{.as_int = (int)node_id}, memory);
      if (!node) {
        mgp::vertex_destroy(node);
        continue;
      }

      mgp::vertex_destroy(node);
      InsertCommunityDetectionRecord(memgraph_graph, result, memory, node_id,
                                     label);
    }
  } catch (const std::exception &e) {
    mgp::result_set_error_msg(result, e.what());
    return;
  }
}

void Update(mgp_list *args, mgp_graph *memgraph_graph, mgp_result *result,
            mgp_memory *memory) {
  try {
    auto created_nodes = mgp::value_get_list(mgp::list_at(args, 0));
    auto created_edges = mgp::value_get_list(mgp::list_at(args, 1));
    auto updated_nodes = mgp::value_get_list(mgp::list_at(args, 2));
    auto updated_edges = mgp::value_get_list(mgp::list_at(args, 3));
    auto deleted_nodes = mgp::value_get_list(mgp::list_at(args, 4));
    auto deleted_edges = mgp::value_get_list(mgp::list_at(args, 5));

    auto graph_type = saved_directedness
                          ? mg_graph::GraphType::kDirectedGraph
                          : mg_graph::GraphType::kUndirectedGraph;
    auto graph = saved_weightedness
                     ? mg_utility::GetWeightedGraphView(
                           memgraph_graph, result, memory, graph_type,
                           saved_weight_property.c_str(), saved_default_weight)
                     : mg_utility::GetGraphView(memgraph_graph, result, memory,
                                                graph_type);

    if (initialized) {
      auto modified_node_ids = mg_utility::GetNodeIDs(created_nodes);
      auto modified_edge_endpoint_ids =
          mg_utility::GetEdgeEndpointIDs(created_edges);

      auto updated_node_ids = mg_utility::GetNodeIDs(updated_nodes);
      modified_node_ids.insert(modified_node_ids.end(),
                               updated_node_ids.begin(),
                               updated_node_ids.end());
      auto updated_edge_endpoint_ids =
          mg_utility::GetEdgeEndpointIDs(updated_edges);
      modified_edge_endpoint_ids.insert(modified_edge_endpoint_ids.end(),
                                        updated_edge_endpoint_ids.begin(),
                                        updated_edge_endpoint_ids.end());

      auto deleted_node_ids = mg_utility::GetNodeIDs(deleted_nodes);
      auto deleted_edge_endpoint_ids =
          mg_utility::GetEdgeEndpointIDs(deleted_edges);

      algorithm.UpdateLabels(graph, modified_node_ids,
                             modified_edge_endpoint_ids, deleted_node_ids,
                             deleted_edge_endpoint_ids);
    } else {
      algorithm.SetLabels(graph);
    }
  } catch (const std::exception &e) {
    mgp::result_set_error_msg(result, e.what());
    return;
  }
}

void Reset(mgp_list *args, mgp_graph *memgraph_graph, mgp_result *result,
           mgp_memory *memory) {
  try {
    ::algorithm = LabelRankT::LabelRankT();
    ::initialized = false;

    ::saved_directedness = false;
    ::saved_weightedness = false;
    ::saved_weight_property = "weight";
    ::saved_default_weight = 1;

    InsertMessageRecord(result, memory,
                        "The algorithm has been successfully reset!");
  } catch (const std::exception &e) {
    InsertMessageRecord(
        result, memory,
        "Reset failed: An exception occurred, please check your module!");
  }
}

extern "C" int mgp_init_module(struct mgp_module *module,
                               struct mgp_memory *memory) {
  auto default_directed = mgp::value_make_bool(0, memory);
  auto default_weighted = mgp::value_make_bool(0, memory);
  auto default_similarity_threshold = mgp::value_make_double(0.7, memory);
  auto default_exponent = mgp::value_make_double(4.0, memory);
  auto default_min_value = mgp::value_make_double(0.1, memory);
  auto default_weight_property = mgp::value_make_string("weight", memory);
  auto default_w_selfloop = mgp::value_make_double(1.0, memory);
  auto default_max_iterations = mgp::value_make_int(100, memory);
  auto default_max_updates = mgp::value_make_int(5, memory);

  try {
    struct mgp_proc *get_proc =
        mgp::module_add_read_procedure(module, "get", Get);
    struct mgp_proc *set_proc =
        mgp::module_add_read_procedure(module, "set", Set);
    struct mgp_proc *update_proc =
        mgp::module_add_read_procedure(module, "update", Update);
    struct mgp_proc *reset_proc =
        mgp::module_add_read_procedure(module, "reset", Reset);

    mgp::proc_add_opt_arg(set_proc, kDirected, mgp::type_bool(),
                          default_directed);
    mgp::proc_add_opt_arg(set_proc, kWeighted, mgp::type_bool(),
                          default_weighted);
    mgp::proc_add_opt_arg(set_proc, kSimilarityThreshold, mgp::type_float(),
                          default_similarity_threshold);
    mgp::proc_add_opt_arg(set_proc, kExponent, mgp::type_float(),
                          default_exponent);
    mgp::proc_add_opt_arg(set_proc, kMinValue, mgp::type_float(),
                          default_min_value);
    mgp::proc_add_opt_arg(set_proc, kWeightProperty, mgp::type_string(),
                          default_weight_property);
    mgp::proc_add_opt_arg(set_proc, kWSelfloop, mgp::type_float(),
                          default_w_selfloop);
    mgp::proc_add_opt_arg(set_proc, kMaxIterations, mgp::type_int(),
                          default_max_iterations);
    mgp::proc_add_opt_arg(set_proc, kMaxUpdates, mgp::type_int(),
                          default_max_updates);

    mgp::proc_add_arg(update_proc, kCreatedVertices,
                      mgp::type_list(mgp::type_node()));
    mgp::proc_add_arg(update_proc, kCreatedEdges,
                      mgp::type_list(mgp::type_relationship()));
    mgp::proc_add_arg(update_proc, kUpdatedVertices,
                      mgp::type_list(mgp::type_node()));
    mgp::proc_add_arg(update_proc, kUpdatedEdges,
                      mgp::type_list(mgp::type_relationship()));
    mgp::proc_add_arg(update_proc, kDeletedVertices,
                      mgp::type_list(mgp::type_node()));
    mgp::proc_add_arg(update_proc, kDeletedEdges,
                      mgp::type_list(mgp::type_relationship()));

    mgp::proc_add_result(get_proc, kFieldNode, mgp::type_node());
    mgp::proc_add_result(get_proc, kFieldCommunityId, mgp::type_int());

    mgp::proc_add_result(set_proc, kFieldNode, mgp::type_node());
    mgp::proc_add_result(set_proc, kFieldCommunityId, mgp::type_int());

    mgp::proc_add_result(reset_proc, kFieldMessage, mgp::type_string());
  } catch (const std::exception &e) {
    mgp::value_destroy(default_directed);
    mgp::value_destroy(default_weighted);
    mgp::value_destroy(default_similarity_threshold);
    mgp::value_destroy(default_exponent);
    mgp::value_destroy(default_min_value);
    mgp::value_destroy(default_weight_property);
    mgp::value_destroy(default_w_selfloop);
    mgp::value_destroy(default_max_iterations);
    mgp::value_destroy(default_max_updates);

    return 1;
  }

  mgp::value_destroy(default_directed);
  mgp::value_destroy(default_weighted);
  mgp::value_destroy(default_similarity_threshold);
  mgp::value_destroy(default_exponent);
  mgp::value_destroy(default_min_value);
  mgp::value_destroy(default_weight_property);
  mgp::value_destroy(default_w_selfloop);
  mgp::value_destroy(default_max_iterations);
  mgp::value_destroy(default_max_updates);

  return 0;
}

extern "C" int mgp_shutdown_module() { return 0; }
