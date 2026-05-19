#include "EscapeAnalysisPass.hpp"
#include "EscapeAnalysisPass_Internal.hpp"
#include <cstddef>
#include "../OptimizationResult.hpp"
#include "../base/OptimizationContext.hpp"

#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/statements/BlockNode.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../ast/nodes/statements/IfNode.hpp"
#include "../../ast/nodes/statements/WhileNode.hpp"
#include "../../ast/nodes/statements/DoWhileNode.hpp"
#include "../../ast/nodes/statements/ForNode.hpp"
#include "../../ast/nodes/statements/ForEachNode.hpp"
#include "../../ast/nodes/statements/TryNode.hpp"
#include "../../ast/nodes/statements/CatchNode.hpp"

#include "../../ast/nodes/functions/FunctionNode.hpp"

#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../ast/nodes/classes/MethodNode.hpp"
#include "../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../ast/nodes/classes/NewNode.hpp"

#include "../../ast/nodes/expressions/LambdaNode.hpp"

#include <chrono>
#include <string>

namespace optimizer::passes::detail
{
    namespace stmt = ast::nodes::statements;
    namespace klass = ast::nodes::classes;

    // ===== Union-find =====

    void FunctionEscapeAnalyzer::ensureTracked(const std::string& name)
    {
        if (ufParent.find(name) == ufParent.end()) ufParent[name] = name;
    }

    std::string FunctionEscapeAnalyzer::findRoot(const std::string& name)
    {
        auto it = ufParent.find(name);
        if (it == ufParent.end()) return {};
        if (it->second != name)
        {
            it->second = findRoot(it->second);
            return it->second;
        }
        return name;
    }

    void FunctionEscapeAnalyzer::unionLocals(const std::string& a, const std::string& b)
    {
        ensureTracked(a);
        ensureTracked(b);
        auto ra = findRoot(a);
        auto rb = findRoot(b);
        if (ra == rb) return;
        ufParent[ra] = rb;
        if (escapedRoots.erase(ra)) escapedRoots.insert(rb);
    }

    void FunctionEscapeAnalyzer::markEscaped(const std::string& name)
    {
        if (ufParent.find(name) == ufParent.end()) return;
        escapedRoots.insert(findRoot(name));
    }

    // ===== Phase 1: collect candidates =====

    // Any `let p = new T(...)` — assignment where the RHS is a direct NewNode.
    // The NewNode pointer is recorded so the flag can be set at the end.
    void FunctionEscapeAnalyzer::collectCandidates(ast::ASTNode* node)
    {
        if (!node) return;

        if (auto* assign = dynamic_cast<stmt::AssignmentNode*>(node))
        {
            // MYT-208: only consider FRESH LOCAL DECLARATIONS — `T x = new T(...)`
            // patterns where `x` is a newly-bound local with a declared type.
            // Reassignments (`x = new T(...)` where x already exists) and
            // class-field assignments (`field = new T(...)` inside a method
            // body, which compiles to SET_FIELD on `this`) carry
            // variableType == VOID per StatementCompiler's branching at
            // `varType != VOID -> emitVariableDeclaration / else
            // emitVariableReassignment`. Promoting those would store the
            // NewNode result into a persistent slot (an existing local that
            // outlives this scope, or a class field), violating the
            // stack-frame lifetime assumption that backs STACK_OBJECT.
            if (assign->getVariableType() == value::ValueType::VOID) {
                // Fall through to recurseForCollection — the RHS may still
                // contain nested candidates inside, but THIS assignment is
                // not itself a candidate.
            }
            else if (auto* newN = dynamic_cast<klass::NewNode*>(assign->getValue()))
            {
                const auto& name = assign->getVariableName();
                // Only record the first NewNode bound to this local in a given
                // function body. Multiple bindings of the same name (rare) all
                // have to be treated as escaping since we can only mark one.
                if (candidates.find(name) == candidates.end())
                {
                    candidates[name] = newN;
                    ensureTracked(name);
                }
                else
                {
                    // Overwrite — both bindings share the same name, either is
                    // risky. Remove from candidates (conservative).
                    candidates.erase(name);
                    markEscaped(name);
                }
            }
        }

        recurseForCollection(node);
    }

    // Recurse into children for candidate collection. Only traverses
    // structural nodes — lambdas and nested functions get their own
    // analysis pass at the top level.
    void FunctionEscapeAnalyzer::recurseForCollection(ast::ASTNode* node)
    {
        if (auto* p = dynamic_cast<stmt::ProgramNode*>(node))
        {
            for (const auto& s : p->getStatements()) collectCandidates(s.get());
            return;
        }
        if (auto* b = dynamic_cast<stmt::BlockNode*>(node))
        {
            for (const auto& s : b->getStatements()) collectCandidates(s.get());
            return;
        }
        if (auto* i = dynamic_cast<stmt::IfNode*>(node))
        {
            collectCandidates(i->getThenStatement());
            collectCandidates(i->getElseStatement());
            return;
        }
        if (auto* w = dynamic_cast<stmt::WhileNode*>(node))
        {
            collectCandidates(w->getBody());
            return;
        }
        if (auto* d = dynamic_cast<stmt::DoWhileNode*>(node))
        {
            collectCandidates(d->getBody());
            return;
        }
        if (auto* f = dynamic_cast<stmt::ForNode*>(node))
        {
            collectCandidates(f->getInitialization());
            collectCandidates(f->getBody());
            return;
        }
        if (auto* fe = dynamic_cast<stmt::ForEachNode*>(node))
        {
            collectCandidates(fe->getBody());
            return;
        }
        if (auto* t = dynamic_cast<stmt::TryNode*>(node))
        {
            collectCandidates(t->getTryBlock());
            for (const auto& c : t->getCatchBlocks())
                collectCandidates(c->getBody());
            collectCandidates(t->getFinallyBlock());
            return;
        }
    }

    // ===== Phase 3: commit flags =====

    void FunctionEscapeAnalyzer::finalize()
    {
        for (auto& kv : candidates)
        {
            const auto& name = kv.first;
            auto* newN = kv.second;
            auto root = findRoot(name);
            if (root.empty()) continue;
            if (escapedRoots.count(root)) continue;
            newN->setIsStackAllocated(true);
            ++promotions;
        }
    }
}

namespace optimizer::passes
{
    namespace
    {
        namespace stmt = ast::nodes::statements;
        namespace func = ast::nodes::functions;
        namespace klass = ast::nodes::classes;

        // Walk the top-level AST and launch a per-body analyzer on every
        // function/method/constructor/lambda. ProgramNode is also treated as
        // a body (scripts can declare locals at top level — see
        // object_alloc.mt, whose hot loop lives at script top level).
        void runOnAllBodies(ast::ASTNode* node, std::size_t& promotions)
        {
            if (!node) return;

            auto analyzeBody = [&](ast::ASTNode* body) {
                detail::FunctionEscapeAnalyzer analyzer(promotions);
                analyzer.analyze(body);
            };

            if (auto* prog = dynamic_cast<stmt::ProgramNode*>(node))
            {
                analyzeBody(prog);
                for (const auto& s : prog->getStatements()) runOnAllBodies(s.get(), promotions);
                return;
            }
            if (auto* cls = dynamic_cast<klass::ClassNode*>(node))
            {
                for (const auto& c : cls->getConstructors()) runOnAllBodies(c.get(), promotions);
                for (const auto& m : cls->getMethods()) runOnAllBodies(m.get(), promotions);
                return;
            }
            if (auto* fn = dynamic_cast<func::FunctionNode*>(node))
            {
                analyzeBody(fn->getBodyPtr());
                return;
            }
            if (auto* m = dynamic_cast<klass::MethodNode*>(node))
            {
                analyzeBody(m->getBodyPtr());
                return;
            }
            if (auto* ctor = dynamic_cast<klass::ConstructorNode*>(node))
            {
                analyzeBody(ctor->getBodyPtr());
                return;
            }
            // LambdaNode bodies are nested inside an enclosing function body;
            // they're analyzed independently when the per-body walk encounters
            // them, via a separate nested-launcher not emitted here (lambdas
            // rarely top-level declare new `let` bindings of candidates in
            // scripts today).
        }
    }

    EscapeAnalysisPass::EscapeAnalysisPass()
        : OptimizationPass("EscapeAnalysis", base::PassType::TRANSFORMATION),
          stackPromotions(0)
    {
        dependencies = {};
    }

    std::unique_ptr<ast::ASTNode> EscapeAnalysisPass::optimize(
        std::unique_ptr<ast::ASTNode> node,
        base::OptimizationContext& context
    )
    {
        stackPromotions = 0;
        transformationCount = 0;

        auto startTime = std::chrono::high_resolution_clock::now();

        runOnAllBodies(node.get(), stackPromotions);

        auto endTime = std::chrono::high_resolution_clock::now();
        executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        transformationCount = stackPromotions;
        if (stackPromotions > 0) context.setModified(true);

        // This pass mutates NewNode flags in place; the returned AST is the same
        // node passed in. Returning the original unique_ptr preserves ownership.
        return node;
    }

    std::string EscapeAnalysisPass::getDescription() const
    {
        return "Escape Analysis: marks non-escaping allocations for stack-promoted "
               "pool-borrowed lifetime (NEW_OBJECT → NEW_STACK)";
    }

    void EscapeAnalysisPass::reportMetrics(OptimizationResult& result) const
    {
        PassMetrics metrics;
        metrics.passName = getName();
        metrics.transformationsApplied = stackPromotions;
        metrics.executionTime = executionTime;
        metrics.modified = (stackPromotions > 0);
        result.addPassMetrics(metrics);
    }

    void EscapeAnalysisPass::reset()
    {
        stackPromotions = 0;
        transformationCount = 0;
        executionTime = std::chrono::milliseconds(0);
    }
}
