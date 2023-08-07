#include "meta.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include "mgp.hpp"

namespace Meta {

namespace {

class Metadata {
 public:
  int64_t node_cnt;
  int64_t relationship_cnt;
  std::unordered_map<std::string, int64_t> labels;
  std::unordered_map<std::string, int64_t> property_key_cnt;
  std::unordered_map<std::string, int64_t> relationship_types;
  std::unordered_map<std::string, int64_t> relationship_types_cnt;

  int64_t get_label_count() const { return static_cast<int64_t>(labels.size()); }
  int64_t get_relationship_type_count() const { return static_cast<int64_t>(relationship_types_cnt.size()); }
  int64_t get_property_key_count() const { return static_cast<int64_t>(property_key_cnt.size()); }

  void reset();
  void update_labels(const mgp::Node &node, int add);
  void update_property_key_cnt(const mgp::Node &node, int add);
  void update_property_key_cnt(const mgp::Relationship &relationship, int add);
  void update_relationship_types(const mgp::Relationship &relationship, int add);
  void update_relationship_types_cnt(const mgp::Relationship &relationship, int add);
};

// Global variable
// This is unsafe in the multithreaded environment, the workaround would be building a thread-safe dynamic storage
// implementation
Metadata metadata;

void insert(std::unordered_map<std::string, int64_t> &map, std::string_view key_view, int64_t add) {
  auto key = std::string(key_view);
  auto iterator = map.find(key);
  if (iterator != map.end()) {
    (*iterator).second += add;
    if ((*iterator).second == 0) {
      map.erase(iterator);
    }
  } else {
    map[key] = add;
  }
}

void insert(std::unordered_map<std::string, int64_t> &map, std::string &key, int64_t add) {
  auto iterator = map.find(key);
  if (iterator != map.end()) {
    (*iterator).second += add;
    if ((*iterator).second == 0) {
      map.erase(iterator);
    }
  } else {
    map[key] = add;
  }
}

void Metadata::reset() {
  node_cnt = relationship_cnt = 0;
  labels.clear();
  property_key_cnt.clear();
  relationship_types.clear();
  relationship_types_cnt.clear();
}

void Metadata::update_labels(const mgp::Node &node, int add) {
  for (const auto &label : node.Labels()) {
    insert(labels, label, add);
  }
}

void Metadata::update_property_key_cnt(const mgp::Node &node, int add) {
  for (const auto &[property, _] : node.Properties()) {
    insert(property_key_cnt, property, add);
  }
}

void Metadata::update_property_key_cnt(const mgp::Relationship &relationship, int add) {
  for (const auto &[property, _] : relationship.Properties()) {
    insert(property_key_cnt, property, add);
  }
}

void Metadata::update_relationship_types(const mgp::Relationship &relationship, int add) {
  const auto type = relationship.Type();

  for (const auto &label : relationship.From().Labels()) {
    auto key = "(:" + std::string(label) + ")-[:" + std::string(type) + "]->()";
    insert(relationship_types, key, add);
  }
  for (const auto &label : relationship.To().Labels()) {
    auto key = "()-[:" + std::string(type) + "]->(:" + std::string(label) + ")";
    insert(relationship_types, key, add);
  }

  insert(relationship_types, "()-[:" + std::string(type) + "]->()", add);
}

void Metadata::update_relationship_types_cnt(const mgp::Relationship &relationship, int add) {
  insert(relationship_types_cnt, relationship.Type(), add);
}

}  // namespace

void Update(mgp_list *args, mgp_graph *memgraph_graph, mgp_result *result, mgp_memory *memory) {
  mgp::memory = memory;
  const auto arguments = mgp::List(args);
  const auto record_factory = mgp::RecordFactory(result);

  try {
    const auto created_objects{arguments[0].ValueList()};
    const auto deleted_objects{arguments[1].ValueList()};
    const auto removed_vertex_properties{arguments[2].ValueList()};
    const auto removed_edge_properties{arguments[3].ValueList()};
    const auto set_vertex_labels{arguments[4].ValueList()};
    const auto removed_vertex_labels{arguments[5].ValueList()};

    for (const auto &object : created_objects) {
      const auto event{object.ValueMap()};

      const auto event_type = event["event_type"].ValueString();
      if (event_type == "created_vertex") {
        Meta::metadata.node_cnt++;
        const auto vertex = event["vertex"].ValueNode();
        Meta::metadata.update_labels(vertex, 1);
        Meta::metadata.update_property_key_cnt(vertex, 1);
      } else if (event_type == "created_edge") {
        Meta::metadata.relationship_cnt++;
        const auto edge = event["edge"].ValueRelationship();
        Meta::metadata.update_relationship_types(edge, 1);
        Meta::metadata.update_relationship_types_cnt(edge, 1);
        Meta::metadata.update_property_key_cnt(edge, 1);
      } else {
        throw mgp::ValueException("Unexpected event type");
      }
    }

    for (const auto &object : deleted_objects) {
      const auto event{object.ValueMap()};

      const auto event_type = event["event_type"].ValueString();
      if (event_type == "deleted_vertex") {
        Meta::metadata.node_cnt--;
        const auto vertex = event["vertex"].ValueNode();
        Meta::metadata.update_labels(vertex, -1);
        Meta::metadata.update_property_key_cnt(vertex, -1);
      } else if (event_type == "deleted_edge") {
        Meta::metadata.relationship_cnt--;
        const auto edge = event["edge"].ValueRelationship();
        Meta::metadata.update_relationship_types(edge, -1);
        Meta::metadata.update_relationship_types_cnt(edge, -1);
        Meta::metadata.update_property_key_cnt(edge, -1);
      } else {
        throw mgp::ValueException("Unexpected event type");
      }
    }

    for (const auto &object : removed_vertex_properties) {
      const auto event{object.ValueMap()};
      insert(Meta::metadata.property_key_cnt, event["key"].ValueString(), -1);
    }

    for (const auto &object : removed_edge_properties) {
      const auto event{object.ValueMap()};
      insert(Meta::metadata.property_key_cnt, event["key"].ValueString(), -1);
    }

    for (const auto &object : set_vertex_labels) {
      const auto event{object.ValueMap()};
      insert(Meta::metadata.labels, event["label"].ValueString(), event["vertices"].ValueList().Size());
    }

    for (const auto &object : removed_vertex_labels) {
      const auto event{object.ValueMap()};
      insert(Meta::metadata.labels, event["label"].ValueString(), -event["vertices"].ValueList().Size());
    }

  } catch (const std::exception &e) {
    record_factory.SetErrorMessage(e.what());
    return;
  }
}

void Stats(mgp_list *args, mgp_graph *memgraph_graph, mgp_result *result, mgp_memory *memory) {
  mgp::memory = memory;
  const auto arguments = mgp::List(args);
  const auto record_factory = mgp::RecordFactory(result);

  try {
    auto record = record_factory.NewRecord();
    mgp::Map stats{};

    int64_t label_count = metadata.get_label_count();
    record.Insert(std::string(kReturnStats1).c_str(), label_count);
    stats.Insert("labelCount", mgp::Value(label_count));

    int64_t relationship_type_count = metadata.get_relationship_type_count();
    record.Insert(std::string(kReturnStats2).c_str(), relationship_type_count);
    stats.Insert("relationshipTypeCount", mgp::Value(relationship_type_count));

    int64_t property_key_count = metadata.get_property_key_count();
    record.Insert(std::string(kReturnStats3).c_str(), property_key_count);
    stats.Insert("propertyKeyCount", mgp::Value(property_key_count));

    record.Insert(std::string(kReturnStats4).c_str(), metadata.node_cnt);
    stats.Insert("nodeCount", mgp::Value(metadata.node_cnt));

    record.Insert(std::string(kReturnStats5).c_str(), metadata.relationship_cnt);
    stats.Insert("relationshipCount", mgp::Value(metadata.relationship_cnt));

    auto create_map = [](const auto &map) {
      mgp::Map result;
      for (const auto &[key, value] : map) {
        result.Insert(key, mgp::Value(value));
      }
      return result;
    };

    auto labels_map = create_map(metadata.labels);
    record.Insert(std::string(kReturnStats6).c_str(), labels_map);
    stats.Insert("labels", mgp::Value(std::move(labels_map)));

    auto relationship_types_map = create_map(metadata.relationship_types);
    record.Insert(std::string(kReturnStats7).c_str(), relationship_types_map);
    stats.Insert("relationshipTypes", mgp::Value(std::move(relationship_types_map)));

    auto relationship_types_count_map = create_map(metadata.relationship_types_cnt);
    record.Insert(std::string(kReturnStats8).c_str(), relationship_types_count_map);
    stats.Insert("relationshipTypesCount", mgp::Value(std::move(relationship_types_count_map)));

    record.Insert(std::string(kReturnStats9).c_str(), stats);

  } catch (const std::exception &e) {
    record_factory.SetErrorMessage(e.what());
    return;
  }
}

void Reset(mgp_list *args, mgp_graph *memgraph_graph, mgp_result *result, mgp_memory *memory) {
  mgp::memory = memory;
  const auto record_factory = mgp::RecordFactory(result);

  try {
    metadata.reset();

  } catch (const std::exception &e) {
    record_factory.SetErrorMessage(e.what());
    return;
  }
}

}  // namespace Meta
