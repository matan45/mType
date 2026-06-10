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
        if (!node)
        {
            return false;
        }

        // Direct return or throw statements
        if (dynamic_cast<ast::ReturnNode*>(node))
        {
            return true;
        }

        if (dynamic_cast<ast::ThrowNode*>(node))
        {
            return true;
        }

        // Block statement
        if (auto* block = dynamic_cast<ast::BlockNode*>(node))
        {
            return blockAlwaysReturns(block);
        }

        // If statement
        if (auto* ifNode = dynamic_cast<ast::IfNode*>(node))
        {
            return ifAlwaysReturns(ifNode);
        }

        // Switch statement
        if (auto* switchNode = dynamic_cast<ast::SwitchNode*>(node))
        {
            return switchAlwaysReturns(switchNode);
        }

        // Try statement
        if (auto* tryNode = dynamic_cast<ast::TryNode*>(node))
        {
            return tryAlwaysReturns(tryNode);
        }

        // Other statements don't guarantee a return
        return false;
    }

    bool ReturnPathValidator::pathAlwaysExitsLoopIteration(ast::ASTNode* node, bool breakExits)
    {
        if (!node)
        {
            return false;
        }

        // Direct exits: return/throw leave the function, continue ends the
        // iteration, break ends the loop (only when it binds to the loop).
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
            return true;
        }

        if (dynamic_cast<ast::BreakNode*>(node))
        {
            return breakExits;
        }

        if (auto* block = dynamic_cast<ast::BlockNode*>(node))
        {
            return blockAlwaysExits(block, breakExits);
        }

        if (auto* ifNode = dynamic_cast<ast::IfNode*>(node))
        {
            return ifAlwaysExits(ifNode, breakExits);
        }

        if (auto* switchNode = dynamic_cast<ast::SwitchNode*>(node))
        {
            return switchAlwaysExits(switchNode);
        }

        if (auto* tryNode = dynamic_cast<ast::TryNode*>(node))
        {
            return tryAlwaysExits(tryNode, breakExits);
        }

        // Other statements (including nested loops) don't guarantee an exit
        return false;
    }

    bool ReturnPathValidator::blockAlwaysReturns(ast::BlockNode* block)
    {
        if (!block)
        {
            return false;
        }

        const auto& statements = block->getStatements();

        // Check if any statement in the block definitely returns
        // Once we find a statement that returns, we know the block returns
        for (const auto& stmt : statements)
        {
            if (pathAlwaysReturns(stmt.get()))
            {
                return true;
            }
        }

        return false;
    }

    bool ReturnPathValidator::ifAlwaysReturns(ast::IfNode* ifNode)
    {
        if (!ifNode)
        {
            return false;
        }

        // An if statement always returns if:
        // 1. Both then and else branches exist AND
        // 2. Both branches always return

        auto* thenBranch = ifNode->getThenStatement();
        auto* elseBranch = ifNode->getElseStatement();

        // If there's no else branch, the if doesn't guarantee a return
        if (!elseBranch)
        {
            return false;
        }

        // Both branches must return
        return pathAlwaysReturns(thenBranch) && pathAlwaysReturns(elseBranch);
    }

    bool ReturnPathValidator::switchAlwaysReturns(ast::SwitchNode* switchNode)
    {
        if (!switchNode)
        {
            return false;
        }

        // A switch always returns if:
        // 1. It has a default case AND
        // 2. All cases (including default) have a return statement

        const auto& cases = switchNode->getCases();
        bool hasDefault = false;

        for (const auto& caseNode : cases)
        {
            // Check if this is a default case
            if (dynamic_cast<ast::DefaultCaseNode*>(caseNode.get()))
            {
                hasDefault = true;
            }

            // Check if regular case node
            if (auto* casePtr = dynamic_cast<ast::CaseNode*>(caseNode.get()))
            {
                // Case statements are a vector - check if any returns
                const auto& statements = casePtr->getStatements();
                bool caseReturns = false;
                for (const auto& stmt : statements)
                {
                    if (pathAlwaysReturns(stmt.get()))
                    {
                        caseReturns = true;
                        break;
                    }
                }

                if (!caseReturns)
                {
                    return false;
                }
            }
            // Check if default case node
            else if (auto* defaultPtr = dynamic_cast<ast::DefaultCaseNode*>(caseNode.get()))
            {
                // Default case statements - same logic
                const auto& statements = defaultPtr->getStatements();
                bool defaultReturns = false;
                for (const auto& stmt : statements)
                {
                    if (pathAlwaysReturns(stmt.get()))
                    {
                        defaultReturns = true;
                        break;
                    }
                }

                if (!defaultReturns)
                {
                    return false;
                }
            }
        }

        // Must have a default case to guarantee all paths are covered
        return hasDefault;
    }

    bool ReturnPathValidator::tryAlwaysReturns(ast::TryNode* tryNode)
    {
        if (!tryNode)
        {
            return false;
        }

        // A try statement always returns if:
        // 1. The try block returns AND all catch blocks return, OR
        // 2. The finally block returns (finally always executes)

        auto* finallyBlock = tryNode->getFinallyBlock();
        if (finallyBlock && pathAlwaysReturns(finallyBlock))
        {
            // Finally block returns - this guarantees a return
            return true;
        }

        // Check if try block and all catch blocks return
        auto* tryBlock = tryNode->getTryBlock();
        if (!pathAlwaysReturns(tryBlock))
        {
            return false;
        }

        const auto& catchBlocks = tryNode->getCatchBlocks();
        for (const auto& catchBlock : catchBlocks)
        {
            if (!pathAlwaysReturns(catchBlock->getBody()))
            {
                return false;
            }
        }

        // Try returns and all catches (if any) return
        // This is true even if there are no catch blocks
        return true;
    }

    bool ReturnPathValidator::blockAlwaysExits(ast::BlockNode* block, bool breakExits)
    {
        if (!block)
        {
            return false;
        }

        for (const auto& stmt : block->getStatements())
        {
            if (pathAlwaysExitsLoopIteration(stmt.get(), breakExits))
            {
                return true;
            }
        }

        return false;
    }

    bool ReturnPathValidator::ifAlwaysExits(ast::IfNode* ifNode, bool breakExits)
    {
        if (!ifNode)
        {
            return false;
        }

        auto* elseBranch = ifNode->getElseStatement();
        if (!elseBranch)
        {
            return false;
        }

        return pathAlwaysExitsLoopIteration(ifNode->getThenStatement(), breakExits)
            && pathAlwaysExitsLoopIteration(elseBranch, breakExits);
    }

    bool ReturnPathValidator::switchAlwaysExits(ast::SwitchNode* switchNode)
    {
        if (!switchNode)
        {
            return false;
        }

        // Inside the switch a `break` binds to the switch itself and falls
        // through to the code after it — it never exits the loop iteration.
        const auto& cases = switchNode->getCases();
        bool hasDefault = false;

        for (const auto& caseNode : cases)
        {
            if (dynamic_cast<ast::DefaultCaseNode*>(caseNode.get()))
            {
                hasDefault = true;
            }

            const std::vector<std::unique_ptr<ast::ASTNode>>* statements = nullptr;
            if (auto* casePtr = dynamic_cast<ast::CaseNode*>(caseNode.get()))
            {
                statements = &casePtr->getStatements();
            }
            else if (auto* defaultPtr = dynamic_cast<ast::DefaultCaseNode*>(caseNode.get()))
            {
                statements = &defaultPtr->getStatements();
            }

            if (statements)
            {
                bool caseExits = false;
                for (const auto& stmt : *statements)
                {
                    if (pathAlwaysExitsLoopIteration(stmt.get(), /*breakExits=*/false))
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
        }

        return hasDefault;
    }

    bool ReturnPathValidator::tryAlwaysExits(ast::TryNode* tryNode, bool breakExits)
    {
        if (!tryNode)
        {
            return false;
        }

        auto* finallyBlock = tryNode->getFinallyBlock();
        if (finallyBlock && pathAlwaysExitsLoopIteration(finallyBlock, breakExits))
        {
            return true;
        }

        if (!pathAlwaysExitsLoopIteration(tryNode->getTryBlock(), breakExits))
        {
            return false;
        }

        for (const auto& catchBlock : tryNode->getCatchBlocks())
        {
            if (!pathAlwaysExitsLoopIteration(catchBlock->getBody(), breakExits))
            {
                return false;
            }
        }

        return true;
    }
}
