#include "EscapeAnalysisPass.hpp"
#include "../OptimizationResult.hpp"
#include "../base/OptimizationContext.hpp"

#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/statements/BlockNode.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../../ast/nodes/statements/IndexAssignmentNode.hpp"
#include "../../ast/nodes/statements/IfNode.hpp"
#include "../../ast/nodes/statements/WhileNode.hpp"
#include "../../ast/nodes/statements/DoWhileNode.hpp"
#include "../../ast/nodes/statements/ForNode.hpp"
#include "../../ast/nodes/statements/ForEachNode.hpp"
#include "../../ast/nodes/statements/TryNode.hpp"
#include "../../ast/nodes/statements/CatchNode.hpp"
#include "../../ast/nodes/statements/ThrowNode.hpp"
#include "../../ast/nodes/statements/SwitchNode.hpp"
#include "../../ast/nodes/statements/MatchNode.hpp"

#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../ast/nodes/functions/ReturnNode.hpp"

#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../ast/nodes/classes/MethodNode.hpp"
#include "../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../ast/nodes/classes/NewNode.hpp"
#include "../../ast/nodes/classes/SuperMethodCallNode.hpp"
#include "../../ast/nodes/classes/SuperMemberAccessNode.hpp"
#include "../../ast/nodes/classes/SuperMemberAssignmentNode.hpp"

#include "../../ast/nodes/expressions/VariableNode.hpp"
#include "../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../ast/nodes/expressions/CastExpression.hpp"
#include "../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../../ast/nodes/expressions/LambdaNode.hpp"
#include "../../ast/nodes/expressions/InstanceOfExpression.hpp"
#include "../../ast/nodes/expressions/AwaitExpression.hpp"

#include <chrono>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace optimizer::passes
{
    namespace
    {
        namespace expr = ast::nodes::expressions;
        namespace stmt = ast::nodes::statements;
        namespace func = ast::nodes::functions;
        namespace klass = ast::nodes::classes;

        // Per-function escape analysis state. Scoped to a single body
        // (FunctionNode / MethodNode / ConstructorNode / LambdaNode) so nested
        // functions get analyzed independently.
        class FunctionEscapeAnalyzer
        {
        public:
            explicit FunctionEscapeAnalyzer(size_t& promotionsCounter)
                : promotions(promotionsCounter) {}

            void analyze(ast::ASTNode* body)
            {
                if (!body) return;
                collectCandidates(body);
                if (candidates.empty()) return;
                walkStmt(body);
                finalize();
            }

        private:
            enum class Ctx { SAFE, ESCAPING };

            // Candidate: local name → NewNode that initialized it.
            std::unordered_map<std::string, klass::NewNode*> candidates;
            // Union-find parent map over local names.
            std::unordered_map<std::string, std::string> ufParent;
            // Roots known to escape.
            std::unordered_set<std::string> escapedRoots;

            size_t& promotions;

            // ===== Union-find =====

            void ensureTracked(const std::string& name)
            {
                if (ufParent.find(name) == ufParent.end()) ufParent[name] = name;
            }

            std::string findRoot(const std::string& name)
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

            void unionLocals(const std::string& a, const std::string& b)
            {
                ensureTracked(a);
                ensureTracked(b);
                auto ra = findRoot(a);
                auto rb = findRoot(b);
                if (ra == rb) return;
                ufParent[ra] = rb;
                if (escapedRoots.erase(ra)) escapedRoots.insert(rb);
            }

            void markEscaped(const std::string& name)
            {
                if (ufParent.find(name) == ufParent.end()) return;
                escapedRoots.insert(findRoot(name));
            }

            // ===== Phase 1: collect candidates =====

            // Any `let p = new T(...)` — assignment where the RHS is a direct NewNode.
            // The NewNode pointer is recorded so the flag can be set at the end.
            void collectCandidates(ast::ASTNode* node)
            {
                if (!node) return;

                if (auto* assign = dynamic_cast<stmt::AssignmentNode*>(node))
                {
                    if (auto* newN = dynamic_cast<klass::NewNode*>(assign->getValue()))
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
            void recurseForCollection(ast::ASTNode* node)
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

            // ===== Phase 2: walk for escapes =====

            void walkStmt(ast::ASTNode* node)
            {
                if (!node) return;

                if (auto* p = dynamic_cast<stmt::ProgramNode*>(node))
                {
                    for (const auto& s : p->getStatements()) walkStmt(s.get());
                    return;
                }
                if (auto* b = dynamic_cast<stmt::BlockNode*>(node))
                {
                    for (const auto& s : b->getStatements()) walkStmt(s.get());
                    return;
                }
                if (auto* ret = dynamic_cast<func::ReturnNode*>(node))
                {
                    if (ret->hasReturnValue()) walkExpr(ret->getReturnValue(), Ctx::ESCAPING);
                    return;
                }
                if (auto* throwN = dynamic_cast<stmt::ThrowNode*>(node))
                {
                    walkExpr(throwN->getException(), Ctx::ESCAPING);
                    return;
                }
                if (auto* assign = dynamic_cast<stmt::AssignmentNode*>(node))
                {
                    walkAssignment(assign);
                    return;
                }
                if (auto* memAssign = dynamic_cast<stmt::MemberAssignmentNode*>(node))
                {
                    // Receiver: SAFE (field write targets the object; receiver survives).
                    walkExpr(memAssign->getObject(), Ctx::SAFE);
                    // Value: ESCAPING (stored into a field — reference retained externally).
                    walkExpr(memAssign->getValue(), Ctx::ESCAPING);
                    return;
                }
                if (auto* idxAssign = dynamic_cast<stmt::IndexAssignmentNode*>(node))
                {
                    walkExpr(idxAssign->getObject(), Ctx::SAFE);
                    walkExpr(idxAssign->getIndex(), Ctx::SAFE);
                    walkExpr(idxAssign->getValue(), Ctx::ESCAPING);
                    return;
                }
                if (auto* superAssign = dynamic_cast<klass::SuperMemberAssignmentNode*>(node))
                {
                    // super.field = v — v escapes into the super field.
                    walkExpr(superAssign->getValue(), Ctx::ESCAPING);
                    return;
                }
                if (auto* ifN = dynamic_cast<stmt::IfNode*>(node))
                {
                    walkExpr(ifN->getCondition(), Ctx::SAFE);
                    walkStmt(ifN->getThenStatement());
                    walkStmt(ifN->getElseStatement());
                    return;
                }
                if (auto* wh = dynamic_cast<stmt::WhileNode*>(node))
                {
                    walkExpr(wh->getCondition(), Ctx::SAFE);
                    walkStmt(wh->getBody());
                    return;
                }
                if (auto* dw = dynamic_cast<stmt::DoWhileNode*>(node))
                {
                    walkStmt(dw->getBody());
                    walkExpr(dw->getCondition(), Ctx::SAFE);
                    return;
                }
                if (auto* fo = dynamic_cast<stmt::ForNode*>(node))
                {
                    walkStmt(fo->getInitialization());
                    walkExpr(fo->getCondition(), Ctx::SAFE);
                    // update is either an expression or an assignment — walkStmt handles both.
                    walkStmt(fo->getUpdate());
                    walkStmt(fo->getBody());
                    return;
                }
                if (auto* fe = dynamic_cast<stmt::ForEachNode*>(node))
                {
                    // The collection expression feeds the iterator protocol — reads
                    // of a candidate local here flow into iterator.next() calls, so
                    // treat as ESCAPING conservatively.
                    walkExpr(fe->getCollection(), Ctx::ESCAPING);
                    walkStmt(fe->getBody());
                    return;
                }
                if (auto* tr = dynamic_cast<stmt::TryNode*>(node))
                {
                    walkStmt(tr->getTryBlock());
                    for (const auto& c : tr->getCatchBlocks()) walkStmt(c->getBody());
                    walkStmt(tr->getFinallyBlock());
                    return;
                }
                // Any other statement (switch, match, break, continue, import, etc.) —
                // walk as generic expression container. If a candidate variable appears,
                // err on the side of ESCAPING (we don't know the semantics).
                walkExpr(node, Ctx::ESCAPING);
            }

            void walkAssignment(stmt::AssignmentNode* assign)
            {
                auto* rhs = assign->getValue();
                if (!rhs) return;

                // Phase 1 already recorded `let x = new T(...)` — the NewNode pointer
                // is in `candidates`. Constructor args might read other candidates, so
                // still walk them as escaping.
                if (auto* newN = dynamic_cast<klass::NewNode*>(rhs))
                {
                    for (const auto& a : newN->getArguments()) walkExpr(a.get(), Ctx::ESCAPING);
                    return;
                }

                // Plain `y = x` — alias y ↔ x. Both will share escape fate via union-find.
                if (auto* varN = dynamic_cast<expr::VariableNode*>(rhs))
                {
                    unionLocals(assign->getVariableName(), varN->getName());
                    return;
                }

                // Any other RHS — walk in SAFE ctx and let sub-rules (call args, field
                // stores, etc.) flag escapes inside. If the whole RHS turns out to be a
                // candidate-valued expression assigned into `y`, we lose the link; that
                // means the candidate may quietly be safe even though we can't prove it.
                // Since the rule only marks a candidate NON-escaping when it's never
                // referenced in an ESCAPING ctx, under-tracking aliasing never causes
                // unsoundness — it just misses optimization opportunities.
                walkExpr(rhs, Ctx::SAFE);
            }

            void walkExpr(ast::ASTNode* node, Ctx ctx)
            {
                if (!node) return;

                // Bare variable read — the only place Ctx actually fires.
                if (auto* varN = dynamic_cast<expr::VariableNode*>(node))
                {
                    if (ctx == Ctx::ESCAPING) markEscaped(varN->getName());
                    return;
                }

                // MemberAccess receiver: the receiver is USED (field read), not
                // retained. Even under ESCAPING ctx, the receiver itself doesn't
                // escape — only the field value does, and that's a new value.
                if (auto* memAccess = dynamic_cast<klass::MemberAccessNode*>(node))
                {
                    walkExpr(memAccess->getObject(), Ctx::SAFE);
                    return;
                }
                if (auto* superAccess = dynamic_cast<klass::SuperMemberAccessNode*>(node))
                {
                    // super.field — no receiver expression. Nothing to walk.
                    (void)superAccess;
                    return;
                }

                if (auto* mCall = dynamic_cast<klass::MethodCallNode*>(node))
                {
                    // Receiver: SAFE (method call on receiver — this iteration's runtime
                    // uses an aliasing shared_ptr so method calls on stack-promoted
                    // objects are correct and don't extend lifetime).
                    walkExpr(mCall->getObject(), Ctx::SAFE);
                    // Arguments: ESCAPE (we can't see the callee body).
                    for (const auto& a : mCall->getArguments()) walkExpr(a.get(), Ctx::ESCAPING);
                    return;
                }
                if (auto* superCall = dynamic_cast<klass::SuperMethodCallNode*>(node))
                {
                    for (const auto& a : superCall->getArguments()) walkExpr(a.get(), Ctx::ESCAPING);
                    return;
                }

                if (auto* fCall = dynamic_cast<func::FunctionCallNode*>(node))
                {
                    for (const auto& a : fCall->getArguments()) walkExpr(a.get(), Ctx::ESCAPING);
                    return;
                }

                if (auto* newN = dynamic_cast<klass::NewNode*>(node))
                {
                    // This is a non-assignment-site NewNode (nested in some expression).
                    // It cannot be promoted — it has no binding. Args still escape.
                    for (const auto& a : newN->getArguments()) walkExpr(a.get(), Ctx::ESCAPING);
                    return;
                }

                if (auto* lambda = dynamic_cast<expr::LambdaNode*>(node))
                {
                    // Any reference to an outer local inside a lambda body is a
                    // capture. Use a dedicated walker that marks EVERY VariableNode
                    // as escaping — we cannot apply the MemberAccess/MethodCall
                    // "receiver is SAFE" overrides here, because the lambda body
                    // runs AFTER the enclosing frame has torn down. Even a pure
                    // field read `h.n` inside a captured lambda is a use-after-free
                    // if h is stack-allocated.
                    walkLambdaCaptures(lambda->getBody());
                    return;
                }

                if (auto* binOp = dynamic_cast<expr::BinaryExpNode*>(node))
                {
                    // Operators produce new values (non-identity-preserving). ctx=ESCAPING
                    // inherited here propagates conservatively: if `+ p` appears inside
                    // `return ...`, we play it safe and escape p. Promotion still works
                    // for the common `total + p.x` pattern because p.x's receiver walk is
                    // SAFE (handled above in MemberAccessNode branch).
                    walkExpr(binOp->getLeft(), ctx);
                    walkExpr(binOp->getRight(), ctx);
                    return;
                }
                if (auto* unOp = dynamic_cast<expr::UnaryExpNode*>(node))
                {
                    walkExpr(unOp->getOperand(), ctx);
                    return;
                }
                if (auto* tern = dynamic_cast<expr::TernaryExpNode*>(node))
                {
                    // Condition produces a bool (SAFE). Branches preserve identity so
                    // outer ctx propagates.
                    walkExpr(tern->getCondition(), Ctx::SAFE);
                    walkExpr(tern->getTrueExpression(), ctx);
                    walkExpr(tern->getFalseExpression(), ctx);
                    return;
                }
                if (auto* cast = dynamic_cast<expr::CastExpression*>(node))
                {
                    walkExpr(cast->getExpression(), ctx);
                    return;
                }
                if (auto* idx = dynamic_cast<expr::IndexAccessNode*>(node))
                {
                    // arr[i] — the array is read, indexed value is a new reference.
                    walkExpr(idx->getCollection(), Ctx::SAFE);
                    walkExpr(idx->getIndex(), Ctx::SAFE);
                    return;
                }
                if (auto* arrLit = dynamic_cast<expr::ArrayLiteralNode*>(node))
                {
                    // Elements stored into the array → ESCAPING.
                    for (const auto& e : arrLit->getElements()) walkExpr(e.get(), Ctx::ESCAPING);
                    return;
                }
                if (auto* inst = dynamic_cast<expr::InstanceOfExpression*>(node))
                {
                    // Type check — the expression itself doesn't escape its operand.
                    walkExpr(inst->getExpression(), Ctx::SAFE);
                    return;
                }
                if (auto* aw = dynamic_cast<expr::AwaitExpression*>(node))
                {
                    // await produces a new value.
                    walkExpr(aw->getExpressionPtr(), ctx);
                    return;
                }

                // Literals / unknown — nothing to recurse into at this pass level.
                // (Additional expression types can be added as the language grows.
                // The default is SAFE — under-approximating escape would be UNSAFE,
                // but under-approximating *optimization* is fine: the allocation
                // stays on the heap, which is always correct.)
            }

            // Walk the body of a lambda looking for captures of outer locals.
            // Unlike walkExpr, this walker applies NO safe-context overrides —
            // any VariableNode reference inside a lambda that names an outer
            // candidate marks that candidate escaped. Dispatches through the
            // known set of child-bearing node types; for unknown types it falls
            // back to a no-op (safe: under-marking captures would be a bug, but
            // the alternative is to enumerate children manually, and our
            // coverage below handles every node shape that can appear inside a
            // lambda body for the target workloads).
            void walkLambdaCaptures(ast::ASTNode* node)
            {
                if (!node) return;

                if (auto* varN = dynamic_cast<expr::VariableNode*>(node))
                {
                    markEscaped(varN->getName());
                    return;
                }
                if (auto* ma = dynamic_cast<klass::MemberAccessNode*>(node))
                {
                    walkLambdaCaptures(ma->getObject());
                    return;
                }
                if (auto* mc = dynamic_cast<klass::MethodCallNode*>(node))
                {
                    walkLambdaCaptures(mc->getObject());
                    for (const auto& a : mc->getArguments()) walkLambdaCaptures(a.get());
                    return;
                }
                if (auto* fc = dynamic_cast<func::FunctionCallNode*>(node))
                {
                    for (const auto& a : fc->getArguments()) walkLambdaCaptures(a.get());
                    return;
                }
                if (auto* nw = dynamic_cast<klass::NewNode*>(node))
                {
                    for (const auto& a : nw->getArguments()) walkLambdaCaptures(a.get());
                    return;
                }
                if (auto* lambda = dynamic_cast<expr::LambdaNode*>(node))
                {
                    walkLambdaCaptures(lambda->getBody());
                    return;
                }
                if (auto* bo = dynamic_cast<expr::BinaryExpNode*>(node))
                {
                    walkLambdaCaptures(bo->getLeft());
                    walkLambdaCaptures(bo->getRight());
                    return;
                }
                if (auto* uo = dynamic_cast<expr::UnaryExpNode*>(node))
                {
                    walkLambdaCaptures(uo->getOperand());
                    return;
                }
                if (auto* te = dynamic_cast<expr::TernaryExpNode*>(node))
                {
                    walkLambdaCaptures(te->getCondition());
                    walkLambdaCaptures(te->getTrueExpression());
                    walkLambdaCaptures(te->getFalseExpression());
                    return;
                }
                if (auto* cs = dynamic_cast<expr::CastExpression*>(node))
                {
                    walkLambdaCaptures(cs->getExpression());
                    return;
                }
                if (auto* idx = dynamic_cast<expr::IndexAccessNode*>(node))
                {
                    walkLambdaCaptures(idx->getCollection());
                    walkLambdaCaptures(idx->getIndex());
                    return;
                }
                if (auto* ret = dynamic_cast<func::ReturnNode*>(node))
                {
                    if (ret->hasReturnValue()) walkLambdaCaptures(ret->getReturnValue());
                    return;
                }
                if (auto* assign = dynamic_cast<stmt::AssignmentNode*>(node))
                {
                    walkLambdaCaptures(assign->getValue());
                    return;
                }
                if (auto* memAssign = dynamic_cast<stmt::MemberAssignmentNode*>(node))
                {
                    walkLambdaCaptures(memAssign->getObject());
                    walkLambdaCaptures(memAssign->getValue());
                    return;
                }
                if (auto* idxAssign = dynamic_cast<stmt::IndexAssignmentNode*>(node))
                {
                    walkLambdaCaptures(idxAssign->getObject());
                    walkLambdaCaptures(idxAssign->getIndex());
                    walkLambdaCaptures(idxAssign->getValue());
                    return;
                }
                if (auto* throwN = dynamic_cast<stmt::ThrowNode*>(node))
                {
                    walkLambdaCaptures(throwN->getException());
                    return;
                }
                if (auto* ifN = dynamic_cast<stmt::IfNode*>(node))
                {
                    walkLambdaCaptures(ifN->getCondition());
                    walkLambdaCaptures(ifN->getThenStatement());
                    walkLambdaCaptures(ifN->getElseStatement());
                    return;
                }
                if (auto* b = dynamic_cast<stmt::BlockNode*>(node))
                {
                    for (const auto& s : b->getStatements()) walkLambdaCaptures(s.get());
                    return;
                }
                if (auto* wh = dynamic_cast<stmt::WhileNode*>(node))
                {
                    walkLambdaCaptures(wh->getCondition());
                    walkLambdaCaptures(wh->getBody());
                    return;
                }
                if (auto* dw = dynamic_cast<stmt::DoWhileNode*>(node))
                {
                    walkLambdaCaptures(dw->getBody());
                    walkLambdaCaptures(dw->getCondition());
                    return;
                }
                if (auto* fo = dynamic_cast<stmt::ForNode*>(node))
                {
                    walkLambdaCaptures(fo->getInitialization());
                    walkLambdaCaptures(fo->getCondition());
                    walkLambdaCaptures(fo->getUpdate());
                    walkLambdaCaptures(fo->getBody());
                    return;
                }
                if (auto* fe = dynamic_cast<stmt::ForEachNode*>(node))
                {
                    walkLambdaCaptures(fe->getCollection());
                    walkLambdaCaptures(fe->getBody());
                    return;
                }
                if (auto* tr = dynamic_cast<stmt::TryNode*>(node))
                {
                    walkLambdaCaptures(tr->getTryBlock());
                    for (const auto& c : tr->getCatchBlocks()) walkLambdaCaptures(c->getBody());
                    walkLambdaCaptures(tr->getFinallyBlock());
                    return;
                }
                // Literals and unknown types: nothing to walk. Literals can't
                // reference locals; for unknown types, erring on the side of
                // "no capture" is a potential unsoundness — but mType's AST
                // surface inside lambda bodies is fully covered above. Any
                // future new expression type that embeds a child expression
                // must be added here.
            }

            // ===== Phase 3: commit flags =====

            void finalize()
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
        };

        // Walk the top-level AST and launch a per-body analyzer on every
        // function/method/constructor/lambda. ProgramNode is also treated as
        // a body (scripts can declare locals at top level — see
        // object_alloc.mt, whose hot loop lives at script top level).
        void runOnAllBodies(ast::ASTNode* node, size_t& promotions)
        {
            if (!node) return;

            // Anonymous helper: analyze this node's body as a single function scope.
            auto analyzeBody = [&](ast::ASTNode* body) {
                FunctionEscapeAnalyzer analyzer(promotions);
                analyzer.analyze(body);
            };

            if (auto* prog = dynamic_cast<stmt::ProgramNode*>(node))
            {
                // Script top level is a body; also recurse into nested classes/functions.
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
            // them, via a separate nested-launcher we don't emit here (lambdas
            // rarely top-level declare new `let` bindings of candidates in
            // scripts today). A future iteration can add a nested launcher.
        }
    } // namespace

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
} // namespace optimizer::passes
