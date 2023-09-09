#include <mgp.hpp>

#include "algorithm/refactor.hpp"

extern "C" int mgp_init_module(struct mgp_module *module, struct mgp_memory *memory) {
  try {
    mgp::memory = memory;

    AddProcedure(Refactor::Categorize, Refactor::kProcedureCategorize, mgp::ProcedureType::Write,
                 {
                     mgp::Parameter(Refactor::kArgumentsCatSourceKey, mgp::Type::String),
                     mgp::Parameter(Refactor::kArgumentsCatRelType, mgp::Type::String),
                     mgp::Parameter(Refactor::kArgumentsCatRelOutgoing, mgp::Type::Bool),
                     mgp::Parameter(Refactor::kArgumentsCatLabelName, mgp::Type::String),
                     mgp::Parameter(Refactor::kArgumentsCatPropKey, mgp::Type::String),
                     mgp::Parameter(Refactor::kArgumentsCopyPropKeys, {mgp::Type::List, mgp::Type::String},
                                    mgp::Value(mgp::List{})),
                 },
                 {mgp::Return(Refactor::kReturnCategorize, mgp::Type::String)}, module, memory);
    AddProcedure(Refactor::CloneNodes, Refactor::kProcedureCloneNodes, mgp::ProcedureType::Write,
                 {mgp::Parameter(Refactor::kArgumentsNodesToClone, {mgp::Type::List, mgp::Type::Node}),
                  mgp::Parameter(Refactor::kArgumentsCloneRels, mgp::Type::Bool, false),
                  mgp::Parameter(Refactor::kArgumentsSkipPropClone, {mgp::Type::List, mgp::Type::String},
                                 mgp::Value(mgp::List{}))},
                 {mgp::Return(Refactor::kReturnClonedNodeId, mgp::Type::Int),
                  mgp::Return(Refactor::kReturnNewNode, mgp::Type::Node)},
                 module, memory);

    AddProcedure(
        Refactor::CloneSubgraphFromPaths, Refactor::kProcedureCSFP, mgp::ProcedureType::Write,
        {mgp::Parameter(Refactor::kArgumentsPath, {mgp::Type::List, mgp::Type::Path}),
         mgp::Parameter(Refactor::kArgumentsConfigMap, {mgp::Type::Map, mgp::Type::Any}, mgp::Value(mgp::Map{}))},
        {mgp::Return(Refactor::kReturnClonedNodeId, mgp::Type::Int),
         mgp::Return(Refactor::kReturnNewNode, mgp::Type::Node)},
        module, memory);

    AddProcedure(
        Refactor::CloneSubgraph, Refactor::kProcedureCloneSubgraph, mgp::ProcedureType::Write,
        {mgp::Parameter(Refactor::kArgumentsNodes, {mgp::Type::List, mgp::Type::Node}),
         mgp::Parameter(Refactor::kArgumentsRels, {mgp::Type::List, mgp::Type::Relationship}, mgp::Value(mgp::List())),
         mgp::Parameter(Refactor::kArgumentsConfigMap, {mgp::Type::Map, mgp::Type::Any}, mgp::Value(mgp::Map{}))},
        {mgp::Return(Refactor::kReturnClonedNodeId, mgp::Type::Int),
         mgp::Return(Refactor::kReturnNewNode, mgp::Type::Node)},
        module, memory);

    AddProcedure(Refactor::RenameLabel, Refactor::kProcedureRenameLabel, mgp::ProcedureType::Write,
                 {mgp::Parameter(Refactor::kRenameLabelArg1, mgp::Type::String),
                  mgp::Parameter(Refactor::kRenameLabelArg2, mgp::Type::String),
                  mgp::Parameter(Refactor::kRenameLabelArg3, {mgp::Type::List, mgp::Type::Node})},
                 {mgp::Return(Refactor::kRenameLabelResult, mgp::Type::Int)}, module, memory);

    AddProcedure(Refactor::RenameNodeProperty, Refactor::kProcedureRenameNodeProperty, mgp::ProcedureType::Write,
                 {mgp::Parameter(Refactor::kRenameNodePropertyArg1, mgp::Type::String),
                  mgp::Parameter(Refactor::kRenameNodePropertyArg2, mgp::Type::String),
                  mgp::Parameter(Refactor::kRenameNodePropertyArg3, {mgp::Type::List, mgp::Type::Node})},
                 {mgp::Return(Refactor::kRenameNodePropertyResult, mgp::Type::Int)}, module, memory);

  } catch (const std::exception &e) {
    return 1;
  }

  return 0;
}

extern "C" int mgp_shutdown_module() { return 0; }
