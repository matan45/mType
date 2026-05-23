#include "AstPositionIndex.hpp"

#include "../../../mType/ast/ASTNode.hpp"
#include "../../../mType/ast/nodes/statements/ProgramNode.hpp"
#include "../../../mType/ast/nodes/statements/BlockNode.hpp"
#include "../../../mType/ast/nodes/statements/AssignmentNode.hpp"
#include "../../../mType/ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../../../mType/ast/nodes/statements/IndexAssignmentNode.hpp"
#include "../../../mType/ast/nodes/statements/IfNode.hpp"
#include "../../../mType/ast/nodes/statements/WhileNode.hpp"
#include "../../../mType/ast/nodes/statements/DoWhileNode.hpp"
#include "../../../mType/ast/nodes/statements/ForNode.hpp"
#include "../../../mType/ast/nodes/statements/ForEachNode.hpp"
#include "../../../mType/ast/nodes/statements/SwitchNode.hpp"
#include "../../../mType/ast/nodes/statements/CaseNode.hpp"
#include "../../../mType/ast/nodes/statements/DefaultCaseNode.hpp"
#include "../../../mType/ast/nodes/statements/MatchNode.hpp"
#include "../../../mType/ast/nodes/statements/MatchCaseNode.hpp"
#include "../../../mType/ast/nodes/statements/MatchDefaultNode.hpp"
#include "../../../mType/ast/nodes/statements/TryNode.hpp"
#include "../../../mType/ast/nodes/statements/CatchNode.hpp"
#include "../../../mType/ast/nodes/statements/ThrowNode.hpp"
#include "../../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../../mType/ast/nodes/classes/FieldNode.hpp"
#include "../../../mType/ast/nodes/classes/MethodNode.hpp"
#include "../../../mType/ast/nodes/classes/ConstructorNode.hpp"
#include "../../../mType/ast/nodes/classes/InterfaceNode.hpp"
#include "../../../mType/ast/nodes/classes/NewNode.hpp"
#include "../../../mType/ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../mType/ast/nodes/classes/MethodCallNode.hpp"
#include "../../../mType/ast/nodes/classes/SuperConstructorCallNode.hpp"
#include "../../../mType/ast/nodes/classes/SuperMethodCallNode.hpp"
#include "../../../mType/ast/nodes/classes/SuperMemberAssignmentNode.hpp"
#include "../../../mType/ast/nodes/classes/ThisConstructorCallNode.hpp"
#include "../../../mType/ast/nodes/functions/FunctionNode.hpp"
#include "../../../mType/ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../mType/ast/nodes/functions/ReturnNode.hpp"
#include "../../../mType/ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../mType/ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../../mType/ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../../mType/ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../../mType/ast/nodes/expressions/CastExpression.hpp"
#include "../../../mType/ast/nodes/expressions/InstanceOfExpression.hpp"
#include "../../../mType/ast/nodes/expressions/AwaitExpression.hpp"
#include "../../../mType/ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../../../mType/ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../../../mType/ast/nodes/expressions/LambdaNode.hpp"

namespace mtype::lsp {

namespace {

// Forward-decls so the dispatch below can recurse.
bool descend(const ast::ASTNode* node,
             const std::function<bool(const ast::ASTNode*)>& visit);

bool descendList(const std::vector<std::unique_ptr<ast::ASTNode>>& nodes,
                 const std::function<bool(const ast::ASTNode*)>& visit) {
    for (const auto& n : nodes) {
        if (descend(n.get(), visit)) return true;
    }
    return false;
}

// One stop on the recursive walk: visit the node, then descend into its
// children. Returns true to short-circuit traversal.
bool descend(const ast::ASTNode* node,
             const std::function<bool(const ast::ASTNode*)>& visit) {
    if (!node) return false;
    if (visit(node)) return true;

    // Containers
    if (auto* p = dynamic_cast<const ast::nodes::statements::ProgramNode*>(node)) {
        return descendList(p->getStatements(), visit);
    }
    if (auto* b = dynamic_cast<const ast::nodes::statements::BlockNode*>(node)) {
        return descendList(b->getStatements(), visit);
    }
    if (auto* c = dynamic_cast<const ast::nodes::classes::ClassNode*>(node)) {
        if (descendList(c->getFields(), visit)) return true;
        if (descendList(c->getConstructors(), visit)) return true;
        if (descendList(c->getMethods(), visit)) return true;
        return false;
    }
    if (auto* i = dynamic_cast<const ast::nodes::classes::InterfaceNode*>(node)) {
        return descendList(i->getMethods(), visit);
    }

    // Function / method / constructor bodies
    if (auto* f = dynamic_cast<const ast::nodes::functions::FunctionNode*>(node)) {
        return descend(f->getBodyPtr(), visit);
    }
    if (auto* m = dynamic_cast<const ast::nodes::classes::MethodNode*>(node)) {
        return descend(m->getBodyPtr(), visit);
    }
    if (auto* k = dynamic_cast<const ast::nodes::classes::ConstructorNode*>(node)) {
        return descend(k->getBodyPtr(), visit);
    }
    if (auto* fld = dynamic_cast<const ast::nodes::classes::FieldNode*>(node)) {
        return descend(fld->getInitialValue(), visit);
    }

    // Control flow
    if (auto* n = dynamic_cast<const ast::nodes::statements::IfNode*>(node)) {
        if (descend(n->getCondition(), visit)) return true;
        if (descend(n->getThenStatement(), visit)) return true;
        if (descend(n->getElseStatement(), visit)) return true;
        return false;
    }
    if (auto* n = dynamic_cast<const ast::nodes::statements::WhileNode*>(node)) {
        if (descend(n->getCondition(), visit)) return true;
        return descend(n->getBody(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::statements::DoWhileNode*>(node)) {
        if (descend(n->getBody(), visit)) return true;
        return descend(n->getCondition(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::statements::ForNode*>(node)) {
        if (descend(n->getInitialization(), visit)) return true;
        if (descend(n->getCondition(), visit)) return true;
        if (descend(n->getUpdate(), visit)) return true;
        return descend(n->getBody(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::statements::ForEachNode*>(node)) {
        if (descend(n->getCollection(), visit)) return true;
        return descend(n->getBody(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::statements::SwitchNode*>(node)) {
        if (descend(n->getExpression(), visit)) return true;
        return descendList(n->getCases(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::statements::CaseNode*>(node)) {
        if (descend(n->getValue(), visit)) return true;
        return descendList(n->getStatements(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::statements::DefaultCaseNode*>(node)) {
        return descendList(n->getStatements(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::statements::MatchNode*>(node)) {
        if (descend(n->getExpression(), visit)) return true;
        return descendList(n->getCases(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::statements::MatchCaseNode*>(node)) {
        if (descend(n->getValueExpression(), visit)) return true;
        return descend(n->getBody(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::statements::MatchDefaultNode*>(node)) {
        return descend(n->getBody(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::statements::TryNode*>(node)) {
        if (descend(n->getTryBlock(), visit)) return true;
        for (const auto& cb : n->getCatchBlocks()) {
            if (descend(cb.get(), visit)) return true;
        }
        return descend(n->getFinallyBlock(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::statements::CatchNode*>(node)) {
        return descend(n->getBody(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::statements::ThrowNode*>(node)) {
        return descend(n->getException(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::functions::ReturnNode*>(node)) {
        return descend(n->getReturnValue(), visit);
    }

    // Assignments
    if (auto* n = dynamic_cast<const ast::nodes::statements::AssignmentNode*>(node)) {
        return descend(n->getValue(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::statements::MemberAssignmentNode*>(node)) {
        if (descend(n->getObject(), visit)) return true;
        return descend(n->getValue(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::statements::IndexAssignmentNode*>(node)) {
        if (descend(n->getObject(), visit)) return true;
        if (descend(n->getIndex(), visit)) return true;
        return descend(n->getValue(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::classes::SuperMemberAssignmentNode*>(node)) {
        return descend(n->getValue(), visit);
    }

    // Expressions
    if (auto* n = dynamic_cast<const ast::nodes::expressions::BinaryExpNode*>(node)) {
        if (descend(n->getLeft(), visit)) return true;
        return descend(n->getRight(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::expressions::UnaryExpNode*>(node)) {
        return descend(n->getOperand(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::expressions::TernaryExpNode*>(node)) {
        if (descend(n->getCondition(), visit)) return true;
        if (descend(n->getTrueExpression(), visit)) return true;
        return descend(n->getFalseExpression(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::expressions::IndexAccessNode*>(node)) {
        if (descend(n->getCollection(), visit)) return true;
        return descend(n->getIndex(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::expressions::CastExpression*>(node)) {
        return descend(n->getExpression(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::expressions::InstanceOfExpression*>(node)) {
        return descend(n->getExpression(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::expressions::AwaitExpression*>(node)) {
        return descend(n->getExpressionPtr(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::expressions::ArrayLiteralNode*>(node)) {
        return descendList(n->getElements(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::expressions::ArrayCreationNode*>(node)) {
        return descendList(n->getSizeExpressions(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::expressions::LambdaNode*>(node)) {
        return descend(n->getBody(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::classes::MemberAccessNode*>(node)) {
        return descend(n->getObject(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::classes::MethodCallNode*>(node)) {
        if (descend(n->getObject(), visit)) return true;
        return descendList(n->getArguments(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::functions::FunctionCallNode*>(node)) {
        return descendList(n->getArguments(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::classes::NewNode*>(node)) {
        return descendList(n->getArguments(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::classes::SuperConstructorCallNode*>(node)) {
        return descendList(n->getArguments(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::classes::SuperMethodCallNode*>(node)) {
        return descendList(n->getArguments(), visit);
    }
    if (auto* n = dynamic_cast<const ast::nodes::classes::ThisConstructorCallNode*>(node)) {
        return descendList(n->getArguments(), visit);
    }

    // Leaves (literals, identifiers, etc.) have no children to descend into.
    return false;
}

// Is the cursor inside the identifier-window anchored at this node's parse
// location? The parser anchors MethodCallNode / FunctionCallNode at the
// start of the method/function name token (ExpressionParser.cpp:591), so the
// match window is [loc.col, loc.col + identifier.length()). All comparisons
// in 1-based AST coordinates.
bool cursorOnIdentifier(const ast::ASTNode& node,
                        int astLine, int astCol,
                        const std::string& identifier) {
    const auto& loc = node.getLocation();
    if (loc.getLine() != astLine) return false;
    int startCol = loc.getColumn();
    int endCol = startCol + static_cast<int>(identifier.size());
    return astCol >= startCol && astCol < endCol;
}

}  // namespace

bool walkAst(const ast::ASTNode* node,
             const std::function<bool(const ast::ASTNode*)>& visit) {
    return descend(node, visit);
}

bool walkAst(const std::vector<std::unique_ptr<ast::ASTNode>>& roots,
             const std::function<bool(const ast::ASTNode*)>& visit) {
    return descendList(roots, visit);
}

const ast::nodes::classes::MethodCallNode* findMethodCallAt(
    const std::vector<std::unique_ptr<ast::ASTNode>>& roots,
    int line, int col,
    const std::string& methodName) {

    const int astLine = line + 1;
    const int astCol = col + 1;
    const ast::nodes::classes::MethodCallNode* match = nullptr;

    walkAst(roots, [&](const ast::ASTNode* n) -> bool {
        auto* call = dynamic_cast<const ast::nodes::classes::MethodCallNode*>(n);
        if (!call) return false;
        if (call->getMethodName() != methodName) return false;
        if (!cursorOnIdentifier(*call, astLine, astCol, methodName)) return false;
        match = call;
        return true;
    });

    return match;
}

const ast::nodes::functions::FunctionCallNode* findFunctionCallAt(
    const std::vector<std::unique_ptr<ast::ASTNode>>& roots,
    int line, int col,
    const std::string& functionName) {

    const int astLine = line + 1;
    const int astCol = col + 1;
    const ast::nodes::functions::FunctionCallNode* match = nullptr;

    walkAst(roots, [&](const ast::ASTNode* n) -> bool {
        auto* call = dynamic_cast<const ast::nodes::functions::FunctionCallNode*>(n);
        if (!call) return false;
        if (call->getFunctionName() != functionName) return false;
        if (!cursorOnIdentifier(*call, astLine, astCol, functionName)) return false;
        match = call;
        return true;
    });

    return match;
}

const ast::ASTNode* findEnclosingCallable(
    const std::vector<std::unique_ptr<ast::ASTNode>>& roots,
    int line, int col) {

    const int astLine = line + 1;
    const ast::ASTNode* enclosing = nullptr;

    // Walk all callables; pick the one whose body's source-anchor line is the
    // largest value <= cursor line. Without source-end ranges this is
    // necessarily approximate, but for typical mType source layouts (one
    // statement per line, function bodies follow the opening brace) it
    // resolves to the right callable for the cases we care about.
    walkAst(roots, [&](const ast::ASTNode* n) -> bool {
        int loc = -1;
        if (auto* f = dynamic_cast<const ast::nodes::functions::FunctionNode*>(n)) {
            loc = f->getLocation().getLine();
        } else if (auto* m = dynamic_cast<const ast::nodes::classes::MethodNode*>(n)) {
            loc = m->getLocation().getLine();
        } else if (auto* k = dynamic_cast<const ast::nodes::classes::ConstructorNode*>(n)) {
            loc = k->getLocation().getLine();
        } else {
            return false;
        }
        if (loc <= astLine) {
            if (!enclosing || loc > enclosing->getLocation().getLine()) {
                enclosing = n;
            }
        }
        return false;
    });

    (void)col;  // unused — we match by line only
    return enclosing;
}

}  // namespace mtype::lsp
