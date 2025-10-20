#include "LoopEvaluator.hpp"
#include "../utils/ScopeGuard.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/ScriptException.hpp"
#include "../../errors/BreakException.hpp"
#include "../../errors/ContinueException.hpp"
#include "../../errors/ReturnException.hpp"
#include "../../value/NativeArray.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"
#include "../../ast/nodes/statements/WhileNode.hpp"
#include "../../ast/nodes/statements/DoWhileNode.hpp"
#include "../../ast/nodes/statements/ForNode.hpp"
#include "../../ast/nodes/statements/ForEachNode.hpp"
#include "../../environment/manager/Scope.hpp"

using namespace errors;

namespace evaluator
{
    namespace statements
    {
        Value LoopEvaluator::evaluateWhile(WhileNode* node)
        {
            if (!exprEvaluator)
            {
                throw TypeException("Expression evaluator not available for while loop evaluation");
            }

            Value lastValue = std::monostate{};
            flowManager->resetLoopFlags();
            flowManager->enterLoop();

            try
            {
                while (true)
                {
                    Value conditionValue = exprEvaluator->evaluate(node->getCondition());
                    if (!exprEvaluator->isTruthy(conditionValue))
                    {
                        break;
                    }

                    try
                    {
                        lastValue = stmtEvaluator->evaluate(node->getBody());
                    }
                    catch (const ContinueException&)
                    {
                        flowManager->resetLoopFlags();
                        continue;
                    }

                    if (context->shouldReturn() || flowManager->isBreaking())
                    {
                        break;
                    }

                    if (flowManager->isContinuing())
                    {
                        flowManager->resetLoopFlags();
                        continue;
                    }
                }
            }
            catch (const BreakException&)
            {
                // Break caught, exit loop normally
            }

            flowManager->resetLoopFlags();
            flowManager->exitLoop();
            return lastValue;
        }

        Value LoopEvaluator::evaluateDoWhile(DoWhileNode* node)
        {
            if (!exprEvaluator)
            {
                throw TypeException("Expression evaluator not available for do-while loop evaluation");
            }

            Value lastValue = std::monostate{};
            flowManager->resetLoopFlags();
            flowManager->enterLoop();

            try
            {
                do
                {
                    try
                    {
                        lastValue = stmtEvaluator->evaluate(node->getBody());
                    }
                    catch (const ContinueException&)
                    {
                        flowManager->resetLoopFlags();
                        // Fall through to condition check
                    }

                    if (context->shouldReturn() || flowManager->isBreaking())
                    {
                        break;
                    }

                    if (flowManager->isContinuing())
                    {
                        flowManager->resetLoopFlags();
                    }

                    Value conditionValue = exprEvaluator->evaluate(node->getCondition());
                    if (!exprEvaluator->isTruthy(conditionValue))
                    {
                        break;
                    }
                }
                while (true);
            }
            catch (const BreakException&)
            {
                // Break caught, exit loop normally
            }

            flowManager->resetLoopFlags();
            flowManager->exitLoop();
            return lastValue;
        }

        Value LoopEvaluator::evaluateFor(ForNode* node)
        {
            if (!exprEvaluator)
            {
                throw TypeException("Expression evaluator not available for for loop evaluation");
            }

            auto env = context->getEnvironment();
            env->enterScope("", environment::manager::ScopeType::BLOCK);

            Value lastValue = std::monostate{};
            flowManager->resetLoopFlags();
            flowManager->enterLoop();

            try
            {
                // Initialize
                if (node->getInitialization())
                {
                    stmtEvaluator->evaluate(node->getInitialization());
                }

                // Loop
                while (true)
                {
                    // Check condition
                    if (node->getCondition())
                    {
                        Value conditionValue = exprEvaluator->evaluate(node->getCondition());
                        if (!exprEvaluator->isTruthy(conditionValue))
                        {
                            break;
                        }
                    }

                    // Execute body
                    try
                    {
                        lastValue = stmtEvaluator->evaluate(node->getBody());
                    }
                    catch (const ContinueException&)
                    {
                        flowManager->resetLoopFlags();
                        // Fall through to update
                    }

                    if (context->shouldReturn() || flowManager->isBreaking())
                    {
                        break;
                    }

                    if (flowManager->isContinuing())
                    {
                        flowManager->resetLoopFlags();
                    }

                    // Update
                    if (node->getUpdate())
                    {
                        stmtEvaluator->evaluate(node->getUpdate());
                    }
                }
            }
            catch (const BreakException&)
            {
                // Break caught, exit loop normally
            }
            catch (...)
            {
                env->exitScope();
                flowManager->exitLoop();
                throw;
            }

            env->exitScope();
            flowManager->resetLoopFlags();
            flowManager->exitLoop();
            return lastValue;
        }

        Value LoopEvaluator::evaluateForEach(ForEachNode* node)
        {
            if (!exprEvaluator)
            {
                throw TypeException("Expression evaluator not available for foreach evaluation");
            }

            // Evaluate the collection to iterate over
            Value collectionValue = exprEvaluator->evaluate(node->getCollection());
            auto env = context->getEnvironment();

            // Create a new scope for the loop
            utils::ScopeGuard scope(env, "foreach", environment::manager::ScopeType::BLOCK);

            // Setup loop tracking and control flow
            flowManager->resetLoopFlags();
            flowManager->enterLoop();

            try
            {
                // Handle native arrays first
                if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(collectionValue))
                {
                    auto array = std::get<std::shared_ptr<value::NativeArray>>(collectionValue);

                    // OPTIMIZATION: Use unchecked access since loop bounds are verified by i < array->size()
                    for (size_t i = 0; i < array->size(); ++i)
                    {
                        try
                        {
                            executeForEachIteration(array->getUnchecked(i), node);
                        }
                        catch (const BreakException&)
                        {
                            // Break caught - exit foreach loop
                            flowManager->resetLoopFlags();
                            flowManager->exitLoop();
                            return std::monostate{};
                        }
                        catch (const ContinueException&)
                        {
                            // Continue caught - reset flags and continue to next iteration
                            flowManager->resetLoopFlags();
                            continue;
                        }
                        catch (const errors::ReturnException& e)
                        {
                            // Return caught - exit loop and propagate return value
                            flowManager->exitLoop();
                            return e.returnValue;
                        }

                        // Check for other control flow interruptions
                        if (flowManager->isBreaking())
                        {
                            break;
                        }
                        if (flowManager->isContinuing())
                        {
                            flowManager->resetLoopFlags();
                        }
                    }
                    flowManager->exitLoop();
                    return std::monostate{};
                }

                // Handle mType collection objects
                if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(collectionValue))
                {
                    auto collection = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(collectionValue);

                    // Extract array from collection (handles Map, Set, List, Stack, Queue)
                    auto iterationArray = extractArrayFromCollection(collection, node);

                    // Iterate through the array
                    // OPTIMIZATION: Use unchecked access since loop bounds are verified by i < iterationArray->size()
                    for (size_t i = 0; i < iterationArray->size(); ++i)
                    {
                        try
                        {
                            executeForEachIteration(iterationArray->getUnchecked(i), node);
                        }
                        catch (const BreakException&)
                        {
                            // Break caught - exit foreach loop
                            flowManager->resetLoopFlags();
                            flowManager->exitLoop();
                            return std::monostate{};
                        }
                        catch (const ContinueException&)
                        {
                            // Continue caught - reset flags and continue to next iteration
                            flowManager->resetLoopFlags();
                            continue;
                        }
                        catch (const errors::ReturnException& e)
                        {
                            // Return caught - exit loop and propagate return value
                            flowManager->exitLoop();
                            return e.returnValue;
                        }

                        // Check for other control flow interruptions
                        if (flowManager->isBreaking())
                        {
                            break;
                        }
                        if (flowManager->isContinuing())
                        {
                            flowManager->resetLoopFlags();
                        }
                    }
                    flowManager->exitLoop();
                    return std::monostate{};
                }

                throw ScriptException("Value is not a valid collection for foreach iteration", node->getLocation());
            }
            catch (const BreakException&)
            {
                // Break caught at foreach level
                flowManager->resetLoopFlags();
            }
            catch (...)
            {
                // Any other exception - make sure to clean up loop state
                flowManager->exitLoop();
                throw;
            }

            // Normal completion - clean up loop state
            flowManager->exitLoop();
            return std::monostate{};
        }

        // ========== Helper Methods for evaluateForEach Refactoring ==========

        void LoopEvaluator::executeForEachIteration(const Value& element, ForEachNode* node)
        {
            auto env = context->getEnvironment();

            // Define the loop variable in this scope
            auto varType = node->getVariableType();
            auto variableDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
                node->getVariableName(), varType, element, false, "");

            env->declareVariable(node->getVariableName(), variableDef);

            // Execute the loop body
            if (node->getBody())
            {
                Value result = stmtEvaluator->evaluate(node->getBody());

                // Handle return - propagate up
                if (context->shouldReturn())
                {
                    flowManager->exitLoop();
                    throw errors::ReturnException(result);
                }
            }

            // Note: Break/Continue are handled via exceptions in the calling loop
        }

        std::shared_ptr<value::NativeArray> LoopEvaluator::extractArrayFromCollection(
            std::shared_ptr<runtimeTypes::klass::ObjectInstance> collection,
            ForEachNode* node)
        {
            auto classDef = collection->getClassDefinition();
            if (!classDef)
            {
                throw ScriptException("Invalid collection object for foreach iteration", node->getLocation());
            }

            std::string className = classDef->getName();

            // For Map collections, iterate over values by default
            if (className.find("Map<") == 0)
            {
                auto getValuesMethod = classDef->findMethod("getValues", 0);
                if (getValuesMethod)
                {
                    context->setCurrentInstance(collection);
                    Value valuesResult = objEvaluator->callMethod(collection, "getValues", {});
                    context->clearCurrentInstance();

                    if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(valuesResult))
                    {
                        return std::get<std::shared_ptr<value::NativeArray>>(valuesResult);
                    }
                }
            }
            else
            {
                // For other collections (Set, List, Stack, Queue), try toArray() method
                auto toArrayMethod = classDef->findMethod("toArray", 0);
                if (toArrayMethod)
                {
                    context->setCurrentInstance(collection);
                    Value arrayResult = objEvaluator->callMethod(collection, "toArray", {});
                    context->clearCurrentInstance();

                    if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(arrayResult))
                    {
                        return std::get<std::shared_ptr<value::NativeArray>>(arrayResult);
                    }
                }
            }

            throw errors::ScriptException(
                "Collection '" + className + "' does not support iteration (missing toArray() or getValues() method)",
                node->getLocation());
        }

    } // namespace statements
} // namespace evaluator
