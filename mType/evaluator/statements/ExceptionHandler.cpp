#include "ExceptionHandler.hpp"
#include "../ExpressionEvaluator.hpp"
#include "../StatementEvaluator.hpp"
#include "../../ast/nodes/statements/TryNode.hpp"
#include "../../ast/nodes/statements/CatchNode.hpp"
#include "../../ast/nodes/statements/ThrowNode.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/ReturnException.hpp"
#include <optional>

namespace evaluator {
namespace statements {

    Value ExceptionHandler::evaluateTry(TryNode* node)
    {
        Value result = std::monostate{};
        bool exceptionCaught = false;
        std::optional<errors::UserException> caughtException;

        try
        {
            // Execute try block
            if (node->getTryBlock())
            {
                result = stmtEvaluator->evaluate(node->getTryBlock());
            }
        }
        catch (errors::UserException& e)
        {
            // User threw an exception with 'throw' statement
            exceptionCaught = true;

            // Try to find a matching catch block
            CatchNode* matchingCatch = findMatchingCatch(e, node->getCatchBlocks());

            if (matchingCatch)
            {
                // Enter new scope for catch block
                context->getEnvironment()->enterScope();

                try
                {
                    // Bind exception to catch variable
                    auto varDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
                        matchingCatch->getVariableName(),
                        ValueType::OBJECT, // Exception is an object type
                        e.getExceptionValue(),
                        false, // not final
                        matchingCatch->getExceptionType()
                    );
                    context->getEnvironment()->declareVariable(matchingCatch->getVariableName(), varDef);

                    // Execute catch block
                    if (matchingCatch->getBody())
                    {
                        result = stmtEvaluator->evaluate(matchingCatch->getBody());
                    }
                }
                catch (...)
                {
                    // Exception in catch block (e.g., ReturnException)
                    context->getEnvironment()->exitScope();
                    // Execute finally block before re-throwing
                    executeFinallyBlock(node->getFinallyBlock());
                    throw;
                }

                context->getEnvironment()->exitScope();
            }
            else
            {
                // No matching catch block - save exception to re-throw after finally
                caughtException = e; // Copy the exception instead of storing a pointer
            }
        }
        catch (errors::ReturnException&)
        {
            // Return exception from try block - execute finally and re-throw
            executeFinallyBlock(node->getFinallyBlock());
            throw;
        }
        catch (...)
        {
            // Other exceptions (BreakException, ContinueException, etc.)
            // Execute finally block and re-throw
            executeFinallyBlock(node->getFinallyBlock());
            throw;
        }

        // Always execute finally block (if present)
        executeFinallyBlock(node->getFinallyBlock());

        // Re-throw uncaught exception
        if (caughtException.has_value())
        {
            throw caughtException.value();
        }

        return result;
    }

    Value ExceptionHandler::evaluateThrow(ThrowNode* node)
    {
        // Evaluate the exception expression
        Value exceptionValue = exprEvaluator->evaluate(node->getException());

        // Get exception type name
        std::string typeName = getExceptionTypeName(exceptionValue);

        // Throw UserException
        throw errors::UserException(exceptionValue, typeName, node->getLocation());
    }

    CatchNode* ExceptionHandler::findMatchingCatch(
        const errors::UserException& exception,
        const std::vector<std::unique_ptr<CatchNode>>& catchBlocks)
    {
        for (const auto& catchBlock : catchBlocks)
        {
            if (exception.matchesCatchType(catchBlock->getExceptionType()))
            {
                return catchBlock.get();
            }
        }

        return nullptr;
    }

    void ExceptionHandler::executeFinallyBlock(ast::ASTNode* finallyBlock)
    {
        if (finallyBlock != nullptr)
        {
            // CRITICAL: Clear the return flag before executing finally block
            // Otherwise, the flag from try/catch block's return will cause
            // executeStatementList to break early and skip the finally block's return!
            bool wasReturned = context->shouldReturn();
            if (wasReturned)
            {
                context->setReturned(false);
            }

            try
            {
                stmtEvaluator->evaluate(finallyBlock);

                // If finally completed normally, restore the return flag
                if (wasReturned && !context->shouldReturn())
                {
                    context->setReturned(true);
                }
            }
            catch (errors::ReturnException&)
            {
                // Finally block has a return statement - it overrides any previous return
                throw;
            }
            catch (...)
            {
                // If finally block throws, that exception takes precedence
                // This matches Java/C# semantics
                throw;
            }
        }
    }

    std::string ExceptionHandler::getExceptionTypeName(const Value& exceptionValue)
    {
        // Check if it's an object instance
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(exceptionValue))
        {
            auto objInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(exceptionValue);
            if (objInstance)
            {
                return objInstance->getTypeName();
            }
            return "NullException";
        }

        // For primitive types (if someone throws a string/int/etc.)
        ValueType type = value::ValueTypeUtils::getValueType(exceptionValue);
        switch (type)
        {
        case ValueType::INT: return "int";
        case ValueType::FLOAT: return "float";
        case ValueType::BOOL: return "bool";
        case ValueType::STRING: return "string";
        default: return "Unknown";
        }
    }

} // namespace statements
} // namespace evaluator
