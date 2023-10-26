#include "algo.hpp"

#include "mgp.hpp"

Algo::PathFinder::PathFinder(const mgp::Node &start_node, const mgp::Node &end_node, int64_t max_length,
                             const mgp::List &rel_types, const mgp::RecordFactory &record_factory)
    : start_node_(start_node), end_node_id_(end_node.Id()), max_length_(max_length), record_factory_(record_factory) {
  UpdateRelationshipDirection(rel_types);
}

void Algo::PathFinder::UpdateRelationshipDirection(const mgp::List &relationship_types) {
  all_incoming_ = false;
  all_outgoing_ = false;

  if (relationship_types.Size() == 0) {  // if no relationships were passed as arguments, all relationships are allowed
    any_outgoing_ = true;
    any_incoming_ = true;
    return;
  }

  bool in_rel = false;
  bool out_rel = false;

  for (const auto &rel : relationship_types) {
    std::string rel_type{std::string(rel.ValueString())};
    bool starts_with = rel_type.starts_with('<');
    bool ends_with = rel_type.ends_with('>');

    // '<' -> all incoming relationship are accepted
    // '>' -> all outgoing relationships are good
    if (rel_type.size() == 1) {
      if (starts_with) {
        any_incoming_ = true;
        in_rel = true;
      } else if (ends_with) {
        any_outgoing_ = true;
        out_rel = true;
      } else {
        rel_direction_[rel_type] = RelDirection::kAny;
        in_rel = out_rel = true;
      }
      continue;
    }

    if (starts_with && ends_with) {  // <type>
      rel_direction_[rel_type.substr(1, rel_type.size() - 2)] = RelDirection::kBoth;
      in_rel = out_rel = true;
    } else if (starts_with) {  // <type
      rel_direction_[rel_type.substr(1)] = RelDirection::kIncoming;
      in_rel = true;
    } else if (ends_with) {  // type>
      rel_direction_[rel_type.substr(0, rel_type.size() - 1)] = RelDirection::kOutgoing;
      out_rel = true;
    } else {  // type
      rel_direction_[rel_type] = RelDirection::kAny;
      in_rel = out_rel = true;
    }
  }

  if (!in_rel) {
    all_outgoing_ = true;
  } else if (!out_rel) {
    all_incoming_ = true;
  }
}

Algo::RelDirection Algo::PathFinder::GetDirection(const std::string &rel_type) const {
  auto it = rel_direction_.find(rel_type);
  if (it == rel_direction_.end()) {
    return RelDirection::kNone;
  }
  return it->second;
}

void Algo::PathFinder::DFS(const mgp::Node &curr_node, mgp::Path &curr_path, std::unordered_set<int64_t> &visited) {
  if (curr_node.Id() == end_node_id_) {
    auto record = record_factory_.NewRecord();
    record.Insert(std::string(kResultAllSimplePaths).c_str(), curr_path);
    return;
  }

  if (static_cast<int64_t>(curr_path.Length()) == max_length_) {
    return;
  }

  visited.insert(curr_node.Id().AsInt());
  std::unordered_set<int64_t> seen;

  auto iterate = [&visited, &seen, &curr_path, this](mgp::Relationships relationships, RelDirection direction,
                                                     bool always_expand) {
    for (const auto relationship : relationships) {
      auto next_node_id =
          direction == RelDirection::kOutgoing ? relationship.To().Id().AsInt() : relationship.From().Id().AsInt();

      if (visited.contains(next_node_id)) {
        continue;
      }

      auto type = std::string(relationship.Type());
      auto wanted_direction = GetDirection(type);

      if (always_expand || wanted_direction == RelDirection::kAny || wanted_direction == direction) {
        curr_path.Expand(relationship);
        DFS(direction == RelDirection::kOutgoing ? relationship.To() : relationship.From(), curr_path, visited);
        curr_path.Pop();
      } else if (wanted_direction == RelDirection::kBoth) {
        if (direction == RelDirection::kOutgoing && seen.contains(relationship.To().Id().AsInt())) {
          curr_path.Expand(relationship);
          DFS(relationship.To(), curr_path, visited);
          curr_path.Pop();
        } else if (direction == RelDirection::kIncoming) {
          seen.insert(relationship.From().Id().AsInt());
        }
      }
    }
  };

  if (!all_outgoing_) {
    iterate(curr_node.InRelationships(), RelDirection::kIncoming, any_incoming_);
  }
  if (!all_incoming_) {
    iterate(curr_node.OutRelationships(), RelDirection::kOutgoing, any_outgoing_);
  }
  visited.erase(curr_node.Id().AsInt());
}

void Algo::PathFinder::FindAllPaths() {
  mgp::Path path{start_node_};
  std::unordered_set<int64_t> visited;
  DFS(start_node_, path, visited);
}

void Algo::AllSimplePaths(mgp_list *args, mgp_graph *memgraph_graph, mgp_result *result, mgp_memory *memory) {
  mgp::MemoryDispatcherGuard guard{memory};
  const auto arguments = mgp::List(args);
  const auto record_factory = mgp::RecordFactory(result);

  try {
    const auto start_node{arguments[0].ValueNode()};
    const auto end_node{arguments[1].ValueNode()};
    const auto rel_types{arguments[2].ValueList()};
    const auto max_nodes{arguments[3].ValueInt()};

    PathFinder pathfinder{start_node, end_node, max_nodes, rel_types, record_factory};
    pathfinder.FindAllPaths();

  } catch (const std::exception &e) {
    record_factory.SetErrorMessage(e.what());
    return;
  }
}

void Algo::Cover(mgp_list *args, mgp_graph *memgraph_graph, mgp_result *result, mgp_memory *memory) {
  mgp::MemoryDispatcherGuard guard{memory};
  const auto arguments = mgp::List(args);
  const auto record_factory = mgp::RecordFactory(result);
  try {
    auto list_nodes = arguments[0].ValueList();
    std::unordered_set<mgp::Node> nodes;
    for (const auto &elem : list_nodes) {
      auto node = elem.ValueNode();
      nodes.insert(node);
    }

    for (const auto &node : nodes) {
      for (const auto rel : node.OutRelationships()) {
        if (nodes.find(rel.To()) != nodes.end()) {
          auto record = record_factory.NewRecord();
          record.Insert(std::string(kCoverRet1).c_str(), rel);
        }
      }
    }

  } catch (const std::exception &e) {
    record_factory.SetErrorMessage(e.what());
    return;
  }
}
