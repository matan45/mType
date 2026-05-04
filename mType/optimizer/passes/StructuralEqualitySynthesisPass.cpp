#include "StructuralEqualitySynthesisPass.hpp"

#include "structural_equality/StructuralEqualityCodegen.hpp"
#include "structural_equality/StructuralEqualityPolicy.hpp"

#include "../OptimizationResult.hpp"
#include "../base/OptimizationContext.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../ast/nodes/classes/MethodNode.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"

namespace optimizer::passes
{
    using namespace ast;
    using structural_equality::ClassRegistry;
    using structural_equality::StructuralEqualityCodegen;
    using structural_equality::StructuralEqualityPolicy;

    namespace
    {
        // Build a "ClassName -> ClassNode*" map for in-program parent walks.
        // Imported classes from other modules are NOT included; the policy
        // soft-skips synthesis for cross-module inheritance.
        ClassRegistry buildClassRegistry(const ProgramNode* program)
        {
            ClassRegistry registry;
            for (const auto& stmt : program->getStatements())
            {
                if (auto* classNode = dynamic_cast<ClassNode*>(stmt.get()))
                {
                    registry[classNode->getClassName()] = classNode;
                }
            }
            return registry;
        }

        // Mutates the class node in place by appending synthesized methods.
        // Returns counts via the (hash, eq) pair.
        std::pair<bool, bool> synthesizeForClass(
            ClassNode* classNode,
            const ClassRegistry& registry)
        {
            if (StructuralEqualityPolicy::isShapeSkippable(classNode))
            {
                return {false, false};
            }

            // Idempotency: the OptimizationPassManager runs passes in a
            // fixed-point loop. If we already added synthesized methods on
            // a previous iteration, do not add duplicates — `hasAny*`
            // includes our previously-synthesized output.
            const bool needsHash = !StructuralEqualityPolicy::hasAnyHashCode(classNode);
            const bool needsEq = !StructuralEqualityPolicy::hasAnyEquals(classNode);
            if (!needsHash && !needsEq) return {false, false};

            // Parent-conflict detection considers ONLY user-declared parent
            // methods (synthetic-skipping); a parent that this same pass
            // synthesized doesn't count as a hand-written contract.
            // May throw InheritanceException — propagate so the user sees a
            // clear compile error instead of silently broken contracts.
            StructuralEqualityPolicy::enforceParentConflictRule(classNode, registry);

            auto ownFields = StructuralEqualityPolicy::collectOwnInstanceFields(classNode);

            // Per-method super-compose gate: only emit `super.hashCode()` /
            // `super.equals()` if the IMMEDIATE parent class actually has
            // that method directly (declared or synthesized). mType's
            // super-dispatch does not chain-walk to find an inherited
            // Object method, so emitting super.X on a parent without it
            // would throw at runtime.
            const bool composeHashWithSuper =
                StructuralEqualityPolicy::parentHasHashCodeMethod(classNode, registry);
            const bool composeEqWithSuper =
                StructuralEqualityPolicy::parentHasEqualsMethod(classNode, registry);

            // Correctness gate: if the class has an in-program parent
            // (hasParentClass + parent in registry, not Object) but the
            // parent has no hashCode/equals (skipped from synthesis because
            // of unsafe fields), the child MUST also skip — otherwise the
            // synthesized child methods would silently ignore parent
            // fields, and two child instances with same own-fields but
            // different parent-fields would wrongly compare equal. Better
            // to fall back to the slow-but-correct Object native on both.
            const bool hasInProgramParent = classNode->hasParentClass() &&
                classNode->getParentClassName() != "Object" &&
                registry.find(classNode->getParentClassName()) != registry.end();
            if (hasInProgramParent && (!composeHashWithSuper || !composeEqWithSuper))
            {
                return {false, false};
            }

            // Nothing to hash AND no super to compose with -> skip.
            if (ownFields.empty() && !composeHashWithSuper && !composeEqWithSuper)
            {
                return {false, false};
            }

            // Phase 1 safety gate: only synthesize when every own field has
            // a type the codegen can handle without ambiguous method-call
            // resolution. Classes with array, generic-parameterized, or
            // non-int primitive fields fall back to the slow Object native.
            if (!StructuralEqualityPolicy::allFieldsSafeForSynthesis(ownFields))
            {
                return {false, false};
            }

            bool synthesizedHash = false;
            bool synthesizedEq = false;
            if (needsHash)
            {
                classNode->addMethod(StructuralEqualityCodegen::buildHashCodeMethod(
                    classNode, ownFields, composeHashWithSuper));
                synthesizedHash = true;
            }
            if (needsEq)
            {
                classNode->addMethod(StructuralEqualityCodegen::buildEqualsMethod(
                    classNode, ownFields, composeEqWithSuper));
                synthesizedEq = true;
            }
            return {synthesizedHash, synthesizedEq};
        }
    }

    StructuralEqualitySynthesisPass::StructuralEqualitySynthesisPass()
        : OptimizationPass("StructuralEqualitySynthesis", base::PassType::TRANSFORMATION)
    {
        dependencies = {};
    }

    std::unique_ptr<ASTNode> StructuralEqualitySynthesisPass::optimize(
        std::unique_ptr<ASTNode> node,
        base::OptimizationContext& context)
    {
        classesSynthesized = 0;
        hashCodeSynthesized = 0;
        equalsSynthesized = 0;
        transformationCount = 0;

        auto startTime = std::chrono::high_resolution_clock::now();

        auto* program = dynamic_cast<ProgramNode*>(node.get());
        if (!program)
        {
            // Synthesis only operates at program scope. If the optimizer is
            // ever invoked on a sub-tree, leave it untouched.
            executionTime = std::chrono::milliseconds(0);
            return node;
        }

        ClassRegistry registry = buildClassRegistry(program);

        for (const auto& stmt : program->getStatements())
        {
            auto* classNode = dynamic_cast<ClassNode*>(stmt.get());
            if (!classNode) continue;

            auto [synthHash, synthEq] = synthesizeForClass(classNode, registry);
            if (synthHash) ++hashCodeSynthesized;
            if (synthEq) ++equalsSynthesized;
            if (synthHash || synthEq) ++classesSynthesized;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        transformationCount = hashCodeSynthesized + equalsSynthesized;

        if (transformationCount > 0)
        {
            context.setModified(true);
        }

        return node;
    }

    std::string StructuralEqualitySynthesisPass::getDescription() const
    {
        return "Structural Equality Synthesis (MYT-274): auto-generates fast "
               "hashCode/equals bytecode bodies for user classes that don't "
               "declare them, replacing the slow string-based Object default.";
    }

    void StructuralEqualitySynthesisPass::reportMetrics(OptimizationResult& result) const
    {
        PassMetrics metrics(
            getName(),
            transformationCount,
            executionTime,
            transformationCount > 0);
        result.addPassMetrics(metrics);
    }

    void StructuralEqualitySynthesisPass::reset()
    {
        classesSynthesized = 0;
        hashCodeSynthesized = 0;
        equalsSynthesized = 0;
        transformationCount = 0;
        executionTime = std::chrono::milliseconds(0);
    }
}
