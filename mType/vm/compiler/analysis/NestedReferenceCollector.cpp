#include "NestedReferenceCollector.hpp"

#include "../../../ast/ASTNode.hpp"
#include "../../../ast/nodes/statements/ProgramNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../../ast/nodes/statements/IfNode.hpp"
#include "../../../ast/nodes/statements/WhileNode.hpp"
#include "../../../ast/nodes/statements/DoWhileNode.hpp"
#include "../../../ast/nodes/statements/ForNode.hpp"
#include "../../../ast/nodes/statements/ForEachNode.hpp"
#include "../../../ast/nodes/statements/TryNode.hpp"
#include "../../../ast/nodes/statements/CatchNode.hpp"
#include "../../../ast/nodes/statements/ThrowNode.hpp"
#include "../../../ast/nodes/statements/BreakNode.hpp"
#include "../../../ast/nodes/statements/ContinueNode.hpp"
#include "../../../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../../../ast/nodes/statements/IndexAssignmentNode.hpp"
#include "../../../ast/nodes/statements/ImportNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../../../ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../../../ast/nodes/expressions/CastExpression.hpp"
#include "../../../ast/nodes/expressions/InstanceOfExpression.hpp"
#include "../../../ast/nodes/expressions/AwaitExpression.hpp"
#include "../../../ast/nodes/expressions/LambdaNode.hpp"
#include "../../../ast/nodes/functions/FunctionNode.hpp"
#include "../../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../ast/nodes/functions/ReturnNode.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/nodes/classes/InterfaceNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../../ast/nodes/classes/NewNode.hpp"

namespace vm::compiler::analysis
{
    namespace
    {
        // Recursive walker. inNestedFn is set while we're inside a non-lambda
        // function/method/constructor body. Every VariableNode read, and every
        // AssignmentNode lhs, encountered while the flag is set adds the
        // identifier name to `names`. Lambdas inherit the flag (a lambda
        // inside a nested fn already counts; a top-level lambda doesn't).
        // `pessimistic` is raised on any node we don't know how to traverse
        // — the caller treats that as "all top-level names referenced" and
        // disables the promotion entirely.
        struct Walker
        {
            std::unordered_set<std::string> names;
            bool pessimistic = false;
            bool inNestedFn = false;

            void walk(ast::ASTNode* node)
            {
                if (!node || pessimistic) return;
                dispatch(node);
            }

            void dispatch(ast::ASTNode* node)
            {
                using namespace ast;

                // Containers — walk children, no flag change.
                if (auto* p = dynamic_cast<ProgramNode*>(node))
                {
                    for (const auto& s : p->getStatements()) walk(s.get());
                    return;
                }
                if (auto* b = dynamic_cast<BlockNode*>(node))
                {
                    for (const auto& s : b->getStatements()) walk(s.get());
                    return;
                }

                // Function/method/constructor bodies enter "nested" mode.
                if (auto* f = dynamic_cast<FunctionNode*>(node))
                {
                    bool prev = inNestedFn;
                    inNestedFn = true;
                    walk(f->getBodyPtr());
                    inNestedFn = prev;
                    return;
                }
                if (auto* m = dynamic_cast<MethodNode*>(node))
                {
                    bool prev = inNestedFn;
                    inNestedFn = true;
                    walk(m->getBodyPtr());
                    inNestedFn = prev;
                    return;
                }
                if (auto* c = dynamic_cast<ConstructorNode*>(node))
                {
                    bool prev = inNestedFn;
                    inNestedFn = true;
                    walk(c->getBodyPtr());
                    inNestedFn = prev;
                    return;
                }
                if (auto* l = dynamic_cast<LambdaNode*>(node))
                {
                    // Lambda bodies always count as "nested context": only
                    // the immediate-outer frame's locals are captured by
                    // captureScopeVariables, so a deeply-nested lambda
                    // (lambda-in-lambda) can't reach a top-level via the
                    // capture chain — it relies on Environment lookup
                    // (LOAD_VAR). Promoting a top-level var that's
                    // referenced anywhere inside any lambda body would
                    // break that lookup. The conservative rule: any name
                    // a lambda body reads is treated as "needs to stay a
                    // global". Top-level lambdas are unaffected (their
                    // captures are unchanged from current behavior because
                    // those vars stay globals).
                    bool prev = inNestedFn;
                    inNestedFn = true;
                    walk(l->getBody());
                    inNestedFn = prev;
                    return;
                }

                if (auto* cls = dynamic_cast<ClassNode*>(node))
                {
                    for (const auto& fn : cls->getMethods())      walk(fn.get());
                    for (const auto& ct : cls->getConstructors()) walk(ct.get());
                    for (const auto& fd : cls->getFields())       walk(fd.get());
                    return;
                }
                if (dynamic_cast<InterfaceNode*>(node)) return;
                if (auto* fld = dynamic_cast<FieldNode*>(node))
                {
                    bool prev = inNestedFn;
                    inNestedFn = true;
                    walk(fld->getInitialValue());
                    inNestedFn = prev;
                    return;
                }

                // Imports — the imported AST is compiled in its own context;
                // its top-level decls become globals via the existing path.
                if (dynamic_cast<ImportNode*>(node)) return;

                // Statements
                if (auto* a = dynamic_cast<AssignmentNode*>(node))
                {
                    if (inNestedFn)
                    {
                        const std::string& nm = a->getVariableName();
                        if (!nm.empty()) names.insert(nm);
                    }
                    walk(a->getValue());
                    return;
                }
                if (auto* m = dynamic_cast<MemberAssignmentNode*>(node))
                {
                    walk(m->getObject());
                    walk(m->getValue());
                    return;
                }
                if (auto* i = dynamic_cast<IndexAssignmentNode*>(node))
                {
                    walk(i->getObject());
                    walk(i->getIndex());
                    walk(i->getValue());
                    return;
                }
                if (auto* i = dynamic_cast<IfNode*>(node))
                {
                    walk(i->getCondition());
                    walk(i->getThenStatement());
                    walk(i->getElseStatement());
                    return;
                }
                if (auto* w = dynamic_cast<WhileNode*>(node))
                {
                    walk(w->getCondition());
                    walk(w->getBody());
                    return;
                }
                if (auto* w = dynamic_cast<DoWhileNode*>(node))
                {
                    walk(w->getCondition());
                    walk(w->getBody());
                    return;
                }
                if (auto* f = dynamic_cast<ForNode*>(node))
                {
                    walk(f->getInitialization());
                    walk(f->getCondition());
                    walk(f->getUpdate());
                    walk(f->getBody());
                    return;
                }
                if (auto* f = dynamic_cast<ForEachNode*>(node))
                {
                    walk(f->getCollection());
                    walk(f->getBody());
                    return;
                }
                if (auto* t = dynamic_cast<TryNode*>(node))
                {
                    walk(t->getTryBlock());
                    for (const auto& c : t->getCatchBlocks()) walk(c.get());
                    walk(t->getFinallyBlock());
                    return;
                }
                if (auto* c = dynamic_cast<CatchNode*>(node))
                {
                    walk(c->getBody());
                    return;
                }
                if (auto* t = dynamic_cast<ThrowNode*>(node))
                {
                    walk(t->getException());
                    return;
                }
                if (dynamic_cast<BreakNode*>(node)) return;
                if (dynamic_cast<ContinueNode*>(node)) return;
                if (auto* r = dynamic_cast<ReturnNode*>(node))
                {
                    walk(r->getReturnValue());
                    return;
                }

                // Expressions
                if (auto* v = dynamic_cast<VariableNode*>(node))
                {
                    if (inNestedFn) names.insert(v->getName());
                    return;
                }
                if (auto* b = dynamic_cast<BinaryOpNode*>(node))
                {
                    walk(b->getLeft());
                    walk(b->getRight());
                    return;
                }
                if (auto* u = dynamic_cast<UnaryOpNode*>(node))
                {
                    walk(u->getOperand());
                    return;
                }
                if (auto* t = dynamic_cast<TernaryOpNode*>(node))
                {
                    walk(t->getCondition());
                    walk(t->getTrueExpression());
                    walk(t->getFalseExpression());
                    return;
                }
                if (auto* call = dynamic_cast<FunctionCallNode*>(node))
                {
                    for (const auto& a : call->getArguments()) walk(a.get());
                    return;
                }
                if (auto* call = dynamic_cast<MethodCallNode*>(node))
                {
                    walk(call->getObject());
                    for (const auto& a : call->getArguments()) walk(a.get());
                    return;
                }
                if (auto* m = dynamic_cast<MemberAccessNode*>(node))
                {
                    walk(m->getObject());
                    return;
                }
                if (auto* n = dynamic_cast<NewNode*>(node))
                {
                    for (const auto& a : n->getArguments()) walk(a.get());
                    return;
                }
                if (auto* i = dynamic_cast<IndexAccessNode*>(node))
                {
                    walk(i->getCollection());
                    walk(i->getIndex());
                    return;
                }
                if (auto* a = dynamic_cast<ArrayLiteralNode*>(node))
                {
                    for (const auto& e : a->getElements()) walk(e.get());
                    return;
                }
                if (auto* a = dynamic_cast<ArrayCreationNode*>(node))
                {
                    for (const auto& e : a->getSizeExpressions()) walk(e.get());
                    return;
                }
                if (auto* c = dynamic_cast<CastExpression*>(node))
                {
                    walk(c->getExpression());
                    return;
                }
                if (auto* i = dynamic_cast<InstanceOfExpression*>(node))
                {
                    walk(i->getExpression());
                    return;
                }
                if (auto* a = dynamic_cast<AwaitExpression*>(node))
                {
                    walk(a->getExpressionPtr());
                    return;
                }

                // Literals — nothing to do.
                if (dynamic_cast<IntegerNode*>(node)) return;
                if (dynamic_cast<FloatNode*>(node)) return;
                if (dynamic_cast<StringNode*>(node)) return;
                if (dynamic_cast<BoolNode*>(node)) return;
                if (dynamic_cast<NullNode*>(node)) return;

                // Unknown node — be safe and disable promotion entirely.
                pessimistic = true;
            }
        };
    }

    std::unordered_set<std::string> NestedReferenceCollector::collect(ast::ASTNode* root)
    {
        Walker w;
        w.walk(root);
        if (w.pessimistic)
        {
            // Sentinel: caller checks for this before consulting normal
            // membership. If the sentinel is present, treat every name as
            // captured and skip promotion.
            std::unordered_set<std::string> all;
            all.insert("*");
            return all;
        }
        return std::move(w.names);
    }
}
