#include "ControlFlowHandler.hpp"
#include "../ExpressionEvaluator.hpp"
#include "../StatementEvaluator.hpp"
#include "../utils/ValueConverter.hpp"
#include "../../ast/nodes/statements/IfNode.hpp"
#include "../../ast/nodes/statements/SwitchNode.hpp"
#include "../../ast/nodes/statements/CaseNode.hpp"
#include "../../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/BreakException.hpp"

using namespace errors;

namespace evaluator
{
    namespace statements
    {
        Value ControlFlowHandler::evaluateIf(IfNode* node)
        {
            if (!exprEvaluator)
            {
                throw TypeException("Expression evaluator not available for if statement evaluation");
            }

            Value conditionValue = exprEvaluator->evaluate(node->getCondition());
            bool isTrue = exprEvaluator->isTruthy(conditionValue);

            if (isTrue)
            {
                return stmtEvaluator->evaluate(node->getThenStatement());
            }
            else if (node->getElseStatement())
            {
                return stmtEvaluator->evaluate(node->getElseStatement());
            }

            return std::monostate{};
        }

        Value ControlFlowHandler::evaluateSwitch(SwitchNode* node)
        {
            if (!exprEvaluator)
            {
                throw TypeException("Expression evaluator not available for switch statement evaluation");
            }

            // Evaluate the switch expression
            Value switchValue = exprEvaluator->evaluate(node->getExpression());

            Value lastValue = std::monostate{};
            bool foundMatch = false;
            bool executeRemaining = false; // For fallthrough behavior

            flowManager->resetLoopFlags(); // Reset any existing break/continue flags

            // Enter breakable context for switch statement
            flowManager->enterBreakableContext();

            try
            {
                // Iterate through all cases
                for (const auto& casePtr : node->getCases())
                {
                    if (auto caseNode = dynamic_cast<CaseNode*>(casePtr.get()))
                    {
                        // Regular case
                        if (!foundMatch && !executeRemaining)
                        {
                            // Check if case value matches switch value
                            Value caseValue = exprEvaluator->evaluate(caseNode->getValue());
                            if (utils::ValueConverter::compareValues(switchValue, caseValue))
                            {
                                foundMatch = true;
                                executeRemaining = true;
                            }
                        }

                        // Execute case statements if we're in fallthrough mode
                        if (executeRemaining)
                        {
                            for (const auto& statement : caseNode->getStatements())
                            {
                                try
                                {
                                    lastValue = stmtEvaluator->evaluate(statement.get());
                                }
                                catch (const BreakException&)
                                {
                                    // Break encountered - exit switch
                                    flowManager->resetLoopFlags();
                                    flowManager->exitBreakableContext();
                                    return lastValue;
                                }

                                // Check for other control flow interruptions
                                if (context->shouldReturn())
                                {
                                    flowManager->exitBreakableContext();
                                    return lastValue;
                                }
                            }
                        }
                    }
                    else if (auto defaultNode = dynamic_cast<DefaultCaseNode*>(casePtr.get()))
                    {
                        // Default case - execute if no match found or if we're in fallthrough
                        if (!foundMatch || executeRemaining)
                        {
                            foundMatch = true; // Mark as found for fallthrough logic
                            executeRemaining = true;

                            for (const auto& statement : defaultNode->getStatements())
                            {
                                try
                                {
                                    lastValue = stmtEvaluator->evaluate(statement.get());
                                }
                                catch (const BreakException&)
                                {
                                    // Break encountered - exit switch
                                    flowManager->resetLoopFlags();
                                    flowManager->exitBreakableContext();
                                    return lastValue;
                                }

                                // Check for other control flow interruptions
                                if (context->shouldReturn())
                                {
                                    flowManager->exitBreakableContext();
                                    return lastValue;
                                }
                            }

                            // Default case has implicit break - exit switch after executing default
                            flowManager->exitBreakableContext();
                            return lastValue;
                        }
                    }
                }
            }
            catch (const BreakException&)
            {
                // Break caught at switch level
                flowManager->resetLoopFlags();
            }

            // Exit breakable context before returning
            flowManager->exitBreakableContext();
            return lastValue;
        }
    } // namespace statements
} // namespace evaluator
