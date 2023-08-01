#include "create.hpp"

void Create::RemoveProperties(mgp_list *args, mgp_graph *memgraph_graph, mgp_result *result, mgp_memory *memory) {
  mgp::memory = memory;
  const auto arguments = mgp::List(args);
  const auto record_factory = mgp::RecordFactory(result);
  try {
    mgp::Graph graph = mgp::Graph(memgraph_graph);
    const mgp::Node node = arguments[0].ValueNode();
    const mgp::List list_keys = arguments[1].ValueList();
    const mgp::Id node_id = node.Id();

    mgp::Node graph_node = graph.GetNodeById(node_id);
    for (auto key : list_keys) {
      std::string key_str(key.ValueString());
      graph_node.RemoveProperty(key_str);
    }
    auto record = record_factory.NewRecord();
    record.Insert(std::string(Create::kReturntRemoveProperties).c_str(), graph_node);

  } catch (const std::exception &e) {
    record_factory.SetErrorMessage(e.what());
    return;
  }
}
