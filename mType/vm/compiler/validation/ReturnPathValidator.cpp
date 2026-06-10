#include "ReturnPathValidator.hpp"
#include "../../../ast/nodes/statements/IfNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/functions/ReturnNode.hpp"
#include "../../../ast/nodes/statements/ThrowNode.hpp"
#include "../../../ast/nodes/statements/SwitchNode.hpp"
#include "../../../ast/nodes/statements/CaseNode.hpp"
#include "../../../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../../../ast/nodes/statements/TryNode.hpp"
#include "../../../ast/nodes/statements/BreakNode.hpp"
#include "../../../ast/nodes/statements/ContinueNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"

namespace vm::compiler::validation
{
    void ReturnPathValidator::validateMethodReturns(
        ast::ASTNode* body,
        value::ValueType returnType,
        const std::string& returnTypeStr,
        const std::string& methodName,
        const errors::SourceLocation& location
    )
    {
        // Void methods don't need validation (implicit return is added by compiler)
        if (returnType == value::ValueType::VOID)
        {
            return;
        }

        // Async void methods (Promise<void>) also don't need validation
        if (returnTypeStr == "Promise<void>")
        {
            return;
        }

        // Check if the method body always returns
        if (!pathAlwaysReturns(body))
        {
            throw errors::TypeException(
                "Method '" + methodName + "' must return a value of type '" +
                returnTypeStr +
                "' on all code paths, but some paths are missing a return statement",
                location
            );
        }
    }

    bool ReturnPathValidator::pathAlwaysReturns(ast::ASTNode* node)
    {
        // Only return/throw count as exits — continue/break do not return.
        return pathAlwaysExits(node, {/*continueExits=*/false, /*breakExits=*/false});
    }

    bool ReturnPathValidator::pathAlwaysExitsLoopIteration(ast::ASTNode* node, bool breakExits)
    {
        // continue always ends the iteration; break does so only when it binds
        // to the loop (breakExits) rather than an enclosing switch.
        return pathAlwaysExits(node, {/*continueExits=*/true, breakExits});
    }

    bool ReturnPathValidator::pathAlwaysExits(ast::ASTNode* node, ExitCriteria crit)
    {
        if (!node)
        {
            return false;
        }

        // return/throw always leave the function; continue/break exit per crit.
        if (dynamic_cast<ast::ReturnNode*>(node))
        {
            return true;
        }

        if (dynamic_cast<ast::ThrowNode*>(node))
        {
            return true;
        }

        if (dynamic_cast<ast::ContinueNode*>(node))
        {
            return crit.continueExits;
        }

        if (dynamic_cast<ast::BreakNode*>(node))
        {
            return crit.breakExits;
        }

        if (auto* block = dynamic_cast<ast::BlockNode*>(node))
        {
            return blockAlwaysExits(block, crit);
        }

        if (auto* ifNode = dynamic_cast<ast::IfNode*>(node))
        {
            return ifAlwaysExits(ifNode, crit);
        }

        if (auto* switchNode = dynamic_cast<ast::SwitchNode*>(node))
        {
            return switchAlwaysExits(switchNode, crit);
        }

        if (auto* tryNode = dynamic_cast<ast::TryNode*>(node))
        {
            return tryAlwaysExits(tryNode, crit);
        }

        // Other statements (including nested loops) don't guarantee an exit
        return false;
    }

    bool ReturnPathValidator::blockAlwaysExits(ast::BlockNode* block, ExitCriteria crit)
    {
        if (!block)
        {
            return false;
        }

        // A block exits once any of its statements definitely exits.
        for (const auto& stmt : block->getStatements())
        {
            if (pathAlwaysExits(stmt.get(), crit))
            {
                return true;
            }
        }

        return false;
    }

    bool ReturnPathValidator::ifAlwaysExits(ast::IfNode* ifNode, ExitCriteria crit)
    {
        if (!ifNode)
        {
            return false;
        }

        // Both branches must exist and exit, otherwise a path falls through.
        auto* elseBranch = ifNode->getElseStatement();
        if (!elseBranch)
        {
            return false;
        }

        return pathAlwaysExits(ifNode->getThenStatement(), crit)
            && pathAlwaysExits(elseBranch, crit);
    }

    bool ReturnPathValidator::switchAlwaysExits(ast::SwitchNode* switchNode, ExitCriteria crit)
    {
        if (!switchNode)
        {
            return false;
        }

        // Inside the switch a `break` binds to the switch itself and falls
        // through to the code after it — it never exits the analyzed path, so
        // recurse with breakExits=false regardless of the caller.
        const ExitCriteria inner{crit.continueExits, /*breakExits=*/false};
        const auto& cases = switchNode->getCases();
        bool hasDefault = false;

        for (const auto& caseNode : cases)
        {
            const std::vector<std::unique_ptr<ast::ASTNode>>* statements = nullptr;
            if (auto* casePtr = dynamic_cast<ast::CaseNode*>(caseNode.get()))
            {
                statements = &casePtr->getStatements();
            }
            else if (auto* defaultPtr = dynamic_cast<ast::DefaultCaseNode*>(caseNode.get()))
            {
                hasDefault = true;
                statements = &defaultPtr->getStatements();
            }

            if (!statements)
            {
                continue;
            }

            bool caseExits = false;
            for (const auto& stmt : *statements)
            {
                if (pathAlwaysExits(stmt.get(), inner))
                {
                    caseExits = true;
                    break;
                }
            }

            if (!caseExits)
            {
                return false;
            }
        }

        // Without a default an unmatched value falls past the switch.
        return hasDefault;
    }

    bool ReturnPathValidator::tryAlwaysExits(ast::TryNode* tryNode, ExitCriteria crit)
    {
        if (!tryNode)
        {
            return false;
        }

        // finally always runs, so if it exits the whole try exits.
        auto* finallyBlock = tryNode->getFinallyBlock();
        if (finallyBlock && pathAlwaysExits(finallyBlock, crit))
        {
            return true;
        }

        // Otherwise the try block and every catch block must exit.
        if (!pathAlwaysExits(tryNode->getTryBlock(), crit))
        {
            return false;
        }

        for (const auto& catchBlock : tryNode->getCatchBlocks())
        {
            if (!pathAlwaysExits(catchBlock->getBody(), crit))
            {
                return false;
            }
        }

        return true;
    }
}
