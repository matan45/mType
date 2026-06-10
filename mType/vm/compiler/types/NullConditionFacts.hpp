#pragma once

#include "NullNarrowingTracker.hpp"
#include "../../../ast/ASTNode.hpp"
#include "../../../ast/NodeClassesDeclaration.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../token/TokenType.hpp"
#include <algorithm>
#include <string>
#include <vector>

namespace vm::compiler::types
{
    struct NullConditionFacts
    {
        std::vector<std::string> whenTrueNonNull;
        std::vector<std::string> whenFalseNonNull;
    };

    inline void addUniqueNullFact(std::vector<std::string>& facts,
                                  const std::string& varName)
    {
        if (varName.empty())
        {
            return;
        }
        if (std::find(facts.begin(), facts.end(), varName) == facts.end())
        {
            facts.push_back(varName);
        }
    }

    inline std::vector<std::string> mergeNullFacts(std::vector<std::string> left,
                                                   const std::vector<std::string>& right)
    {
        for (const auto& fact : right)
        {
            addUniqueNullFact(left, fact);
        }
        return left;
    }

    inline std::vector<std::string> intersectNullFacts(
        const std::vector<std::string>& left,
        const std::vector<std::string>& right)
    {
        std::vector<std::string> result;
        for (const auto& fact : left)
        {
            if (std::find(right.begin(), right.end(), fact) != right.end())
            {
                addUniqueNullFact(result, fact);
            }
        }
        return result;
    }

    inline bool extractVariableNullComparison(ast::BinaryOpNode* binExpr,
                                              std::string& varName,
                                              token::TokenType& op)
    {
        if (!binExpr)
        {
            return false;
        }

        auto* left = binExpr->getLeft();
        auto* right = binExpr->getRight();
        op = binExpr->getOperator();
        if (op != token::TokenType::EQUALS && op != token::TokenType::NOT_EQUALS)
        {
            return false;
        }

        const bool leftIsNull = dynamic_cast<ast::NullNode*>(left) != nullptr;
        const bool rightIsNull = dynamic_cast<ast::NullNode*>(right) != nullptr;
        auto* leftVar = dynamic_cast<ast::VariableNode*>(left);
        auto* rightVar = dynamic_cast<ast::VariableNode*>(right);

        if (rightIsNull && leftVar)
        {
            varName = leftVar->getName();
            return true;
        }
        if (leftIsNull && rightVar)
        {
            varName = rightVar->getName();
            return true;
        }
        return false;
    }

    inline NullConditionFacts analyzeNullCondition(ast::ASTNode* node)
    {
        auto* binExpr = dynamic_cast<ast::BinaryOpNode*>(node);
        if (!binExpr)
        {
            return {};
        }

        const token::TokenType op = binExpr->getOperator();
        if (op == token::TokenType::AND)
        {
            NullConditionFacts left = analyzeNullCondition(binExpr->getLeft());
            NullConditionFacts right = analyzeNullCondition(binExpr->getRight());
            NullConditionFacts result;
            result.whenTrueNonNull = mergeNullFacts(left.whenTrueNonNull,
                                                    right.whenTrueNonNull);
            result.whenFalseNonNull = intersectNullFacts(
                left.whenFalseNonNull,
                mergeNullFacts(left.whenTrueNonNull, right.whenFalseNonNull));
            return result;
        }

        if (op == token::TokenType::OR)
        {
            NullConditionFacts left = analyzeNullCondition(binExpr->getLeft());
            NullConditionFacts right = analyzeNullCondition(binExpr->getRight());
            NullConditionFacts result;
            result.whenTrueNonNull = intersectNullFacts(
                left.whenTrueNonNull,
                mergeNullFacts(left.whenFalseNonNull, right.whenTrueNonNull));
            result.whenFalseNonNull = mergeNullFacts(left.whenFalseNonNull,
                                                     right.whenFalseNonNull);
            return result;
        }

        std::string varName;
        token::TokenType comparisonOp;
        if (!extractVariableNullComparison(binExpr, varName, comparisonOp))
        {
            return {};
        }

        NullConditionFacts result;
        if (comparisonOp == token::TokenType::NOT_EQUALS)
        {
            addUniqueNullFact(result.whenTrueNonNull, varName);
        }
        else
        {
            addUniqueNullFact(result.whenFalseNonNull, varName);
        }
        return result;
    }

    class ScopedNullNarrowing
    {
    public:
        // MYT-381: forceScope pushes a narrowing scope even with no facts, so
        // guard-clause narrowing registered inside a loop body cannot outlive
        // the loop (the break path lands right after it).
        ScopedNullNarrowing(NullNarrowingTracker& tracker,
                            const std::vector<std::string>& varNames,
                            bool forceScope = false)
            : tracker_(tracker)
            , active_(forceScope || !varNames.empty())
        {
            if (!active_)
            {
                return;
            }

            tracker_.enterScope();
            for (const auto& varName : varNames)
            {
                tracker_.narrowToNonNull(varName);
            }
        }

        ~ScopedNullNarrowing()
        {
            if (active_)
            {
                tracker_.exitScope();
            }
        }

        ScopedNullNarrowing(const ScopedNullNarrowing&) = delete;
        ScopedNullNarrowing& operator=(const ScopedNullNarrowing&) = delete;

    private:
        NullNarrowingTracker& tracker_;
        bool active_;
    };
}
