#include "EscapeAnalysisPass_Internal.hpp"

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

#include "../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../ast/nodes/functions/ReturnNode.hpp"

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

namespace optimizer::passes::detail
{
    namespace expr = ast::nodes::expressions;
    namespace stmt = ast::nodes::statements;
    namespace func = ast::nodes::functions;
    namespace klass = ast::nodes::classes;

    // ===== Phase 2: walk for escapes =====

    void FunctionEscapeAnalyzer::walkStmt(ast::ASTNode* node)
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

    void FunctionEscapeAnalyzer::walkAssignment(stmt::AssignmentNode* assign)
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

        // Plain `y = x` — alias y ↔ x. Both share escape fate via union-find.
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

    void FunctionEscapeAnalyzer::walkExpr(ast::ASTNode* node, Ctx ctx)
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
            // MYT-208: receiver is ESCAPING. The analyzer can't tell at
            // AST time whether the resolved method is bytecode (where
            // STACK_OBJECT receivers route through frame.thisInstanceRaw
            // safely) or native (e.g. getClass / hashCode / equals — these
            // call ScriptAPI / TypeExecutor helpers that historically
            // extract a shared_ptr<ObjectInstance> and may store it
            // beyond the call). Treating receivers as ESCAPING is
            // conservative but correct; field-read paths
            // (MemberAccessNode below) still use SAFE so nested-helper
            // patterns (e.g. distanceSq's `a.x - b.x`) stay optimised.
            walkExpr(mCall->getObject(), Ctx::ESCAPING);
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
            // Non-assignment-site NewNode (nested in some expression).
            // It cannot be promoted — no binding. Args still escape.
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
            walkExpr(aw->getExpressionPtr(), ctx);
            return;
        }

        // Literals / unknown — nothing to recurse into. The default is SAFE —
        // under-approximating escape would be UNSAFE, but under-approximating
        // optimization is fine: the allocation stays on the heap, which is
        // always correct.
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
    void FunctionEscapeAnalyzer::walkLambdaCaptures(ast::ASTNode* node)
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
        // Literals and unknown types: nothing to walk. Any future new
        // expression type that embeds a child expression must be added here.
    }
}
