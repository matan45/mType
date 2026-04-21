#include "DefiniteAssignmentAnalyzer.hpp"

#include "../../../ast/ASTNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"

namespace vm::compiler::analysis
{
    namespace
    {
        bool isThisReceiver(ast::ASTNode* node)
        {
            if (auto* var = dynamic_cast<ast::nodes::expressions::VariableNode*>(node))
            {
                return var->getName() == "this";
            }
            return false;
        }

        // Sound "expression is this-independent" check: recognises only a small
        // allow-list of expression shapes; everything unknown returns false.
        // Returning false is always safe — it just means we won't mark the
        // surrounding assignment as definite.
        bool isThisIndependent(ast::ASTNode* expr)
        {
            if (!expr) return true;

            if (dynamic_cast<ast::nodes::expressions::IntegerNode*>(expr)) return true;
            if (dynamic_cast<ast::nodes::expressions::FloatNode*>(expr))   return true;
            if (dynamic_cast<ast::nodes::expressions::BoolNode*>(expr))    return true;
            if (dynamic_cast<ast::nodes::expressions::StringNode*>(expr))  return true;
            if (dynamic_cast<ast::nodes::expressions::NullNode*>(expr))    return true;

            if (auto* var = dynamic_cast<ast::nodes::expressions::VariableNode*>(expr))
            {
                // `this` as a whole value is technically readable but isn't a
                // field read — still reject it for MVP because methods down
                // the line could capture it. Parameters and locals are fine.
                return var->getName() != "this";
            }

            if (auto* bin = dynamic_cast<ast::nodes::expressions::BinaryExpNode*>(expr))
            {
                return isThisIndependent(bin->getLeft())
                    && isThisIndependent(bin->getRight());
            }

            if (auto* un = dynamic_cast<ast::nodes::expressions::UnaryExpNode*>(expr))
            {
                return isThisIndependent(un->getOperand());
            }

            if (auto* tern = dynamic_cast<ast::nodes::expressions::TernaryExpNode*>(expr))
            {
                return isThisIndependent(tern->getCondition())
                    && isThisIndependent(tern->getTrueExpression())
                    && isThisIndependent(tern->getFalseExpression());
            }

            // Anything else (MemberAccess, MethodCall, NewNode, cast, index,
            // lambda, etc.) is not provably this-independent. Reject.
            return false;
        }
    }

    std::unordered_set<std::string> DefiniteAssignmentAnalyzer::analyzeConstructor(ast::ASTNode* body)
    {
        std::unordered_set<std::string> assigned;
        if (!body) return assigned;

        auto* block = dynamic_cast<ast::nodes::statements::BlockNode*>(body);
        if (!block) return assigned;

        for (const auto& stmtUniq : block->getStatements())
        {
            auto* stmt = stmtUniq.get();
            auto* memberAssign = dynamic_cast<ast::nodes::statements::MemberAssignmentNode*>(stmt);
            if (!memberAssign)
            {
                // Any non-member-assignment statement means some code we can't
                // characterise runs mid-constructor. Abandon the set — a later
                // read of an already-"assigned" field would then see the
                // constructor's own value, which is fine, but super()/this()/
                // method-call side effects could also run and read fields we
                // haven't touched yet (via internal this references). Returning
                // what we've accumulated so far is still safe (those fields
                // were definitely written before anything in the tail could
                // run), but for MVP we take the stricter route: bail.
                return {};
            }

            if (!isThisReceiver(memberAssign->getObject())) return {};
            if (!isThisIndependent(memberAssign->getValue())) return {};

            assigned.insert(memberAssign->getMemberName());
        }

        return assigned;
    }

    std::optional<std::vector<std::pair<std::string, size_t>>>
    DefiniteAssignmentAnalyzer::analyzeTrivialCtor(ast::ASTNode* body,
                                                   const std::vector<std::string>& paramNames)
    {
        std::vector<std::pair<std::string, size_t>> result;
        if (!body) return result;  // empty body is trivially "trivial"

        auto* block = dynamic_cast<ast::nodes::statements::BlockNode*>(body);
        if (!block) return std::nullopt;

        for (const auto& stmtUniq : block->getStatements())
        {
            auto* memberAssign =
                dynamic_cast<ast::nodes::statements::MemberAssignmentNode*>(stmtUniq.get());
            if (!memberAssign) return std::nullopt;

            if (!isThisReceiver(memberAssign->getObject())) return std::nullopt;

            // RHS must be a direct parameter reference. Literals and
            // arithmetic don't qualify — we need to evaluate-free copy the
            // arg into the slot, and the fast path doesn't run expression
            // bytecode.
            auto* var = dynamic_cast<ast::nodes::expressions::VariableNode*>(
                memberAssign->getValue());
            if (!var) return std::nullopt;
            if (var->getName() == "this") return std::nullopt;

            size_t paramIdx = SIZE_MAX;
            for (size_t i = 0; i < paramNames.size(); ++i)
            {
                if (paramNames[i] == var->getName()) { paramIdx = i; break; }
            }
            if (paramIdx == SIZE_MAX) return std::nullopt;  // not a param

            result.emplace_back(memberAssign->getMemberName(), paramIdx);
        }

        return result;
    }
}
