#pragma once
#include <memory>
#include "../../../ast/nodes/annotations/AnnotationNode.hpp"
#include "../../../environment/Environment.hpp"
#include "../../bytecode/BytecodeProgram.hpp"

namespace vm::compiler::registration
{
    // MYT-110: populate AnnotationData from an AST AnnotationNode (typed args).
    // Single source of truth for ClassRegistrar and FunctionRegistrar.
    void populateAnnotationData(
        bytecode::BytecodeProgram::AnnotationData& out,
        const ast::nodes::annotations::AnnotationNode& node);

    // MYT-109 + MYT-110: returns true if the annotation should be retained at
    // runtime and in bytecode. SOURCE-retention annotations are dropped before
    // class/function registration so they are invisible to reflection and are
    // not serialized into the .mtc file.
    //
    // `@Retention(SOURCE)` may land in the AnnotationNode under either:
    //   * "policy" — explicit named arg `@Retention(policy = SOURCE)`
    //   * "__positional__" — positional shorthand `@Retention(SOURCE)`
    bool shouldRetainAnnotation(
        const ast::nodes::annotations::AnnotationNode& annotation,
        const std::shared_ptr<environment::Environment>& environment);
}
