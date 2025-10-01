#include "StatementEvaluator.hpp"
#include "statements/LoopEvaluator.hpp"
#include "statements/DeclarationHandler.hpp"
#include "statements/ControlFlowHandler.hpp"
#include "statements/ImportAndFunctionHandler.hpp"
#include "validation/TypeValidator.hpp"
#include "../constants/LambdaConstants.hpp"
#include "../services/ImportManager.hpp"
#include <filesystem>
#include <iostream>
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../ast/nodes/statements/DeclarationNode.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/statements/IfNode.hpp"
#include "../ast/nodes/statements/WhileNode.hpp"
#include "../ast/nodes/statements/DoWhileNode.hpp"
#include "../ast/nodes/statements/ForNode.hpp"
#include "../ast/nodes/statements/SwitchNode.hpp"
#include "../ast/nodes/statements/CaseNode.hpp"
#include "../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../ast/nodes/statements/ImportNode.hpp"
#include "../ast/nodes/statements/NativeFunctionNode.hpp"
#include "../ast/nodes/functions/FunctionNode.hpp"
#include "../ast/nodes/functions/ReturnNode.hpp"
#include "../environment/manager/Scope.hpp"
#include "../exception/BreakException.hpp"
#include "../exception/ContinueException.hpp"
#include "../exception/ReturnException.hpp"
#include "../errors/TypeException.hpp"
#include "../errors/UndefinedException.hpp"
#include "../errors/RuntimeException.hpp"
#include "../errors/ScriptException.hpp"
#include "../errors/EnvironmentException.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include "../ast/nodes/statements/ContinueNode.hpp"
#include "../ast/nodes/statements/ForEachNode.hpp"
#include "utils/ScopeGuard.hpp"
#include "ExpressionEvaluator.hpp"
#include "ObjectEvaluator.hpp"
#include "../value/NativeArray.hpp"
#include "../value/LambdaValue.hpp"
#include "../runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"

namespace evaluator
{
    using namespace errors;
    using namespace exception;
    using namespace runtimeTypes::global;
    using namespace environment::manager;
    using namespace services;
    using namespace utils;

    StatementEvaluator::StatementEvaluator(std::shared_ptr<EvaluationContext> ctx)
        : context(ctx), flowManager(std::make_unique<ControlFlowManager>()),
          loopEvaluator(std::make_unique<statements::LoopEvaluator>(ctx, flowManager.get())),
          declarationHandler(std::make_unique<statements::DeclarationHandler>(ctx)),
          controlFlowHandler(std::make_unique<statements::ControlFlowHandler>(ctx, flowManager.get())),
          importAndFunctionHandler(std::make_unique<statements::ImportAndFunctionHandler>(ctx)),
          exprEvaluator(nullptr), objEvaluator(nullptr)
    {
        // Set back-references so handlers can call back to StatementEvaluator
        loopEvaluator->setStatementEvaluator(this);
        controlFlowHandler->setStatementEvaluator(this);
        importAndFunctionHandler->setStatementEvaluator(this);
    }

    StatementEvaluator::~StatementEvaluator() = default;

    Value StatementEvaluator::evaluate(ASTNode* node)
    {
        if (!node)
        {
            return std::monostate{};
        }

        // Try to handle with this evaluator first
        if (!canHandle(node))
        {
            // Delegate to other evaluators if this one can't handle it
            if (exprEvaluator && exprEvaluator->canHandle(node))
            {
                return exprEvaluator->evaluate(node);
            }
            else if (objEvaluator && objEvaluator->canHandle(node))
            {
                return objEvaluator->evaluate(node);
            }
            return std::monostate{};
        }

        // Dispatch to appropriate evaluation method based on node type
        if (auto progNode = dynamic_cast<ProgramNode*>(node))
        {
            return evaluateProgramNode(progNode);
        }
        if (auto blockNode = dynamic_cast<BlockNode*>(node))
        {
            return evaluateBlockNode(blockNode);
        }
        if (auto declNode = dynamic_cast<DeclarationNode*>(node))
        {
            return evaluateDeclarationNode(declNode);
        }
        if (auto assignNode = dynamic_cast<AssignmentNode*>(node))
        {
            return evaluateAssignmentNode(assignNode);
        }
        if (auto ifNode = dynamic_cast<IfNode*>(node))
        {
            return evaluateIfNode(ifNode);
        }
        if (auto whileNode = dynamic_cast<WhileNode*>(node))
        {
            return evaluateWhileNode(whileNode);
        }
        if (auto doWhileNode = dynamic_cast<DoWhileNode*>(node))
        {
            return evaluateDoWhileNode(doWhileNode);
        }
        if (auto forNode = dynamic_cast<ForNode*>(node))
        {
            return evaluateForNode(forNode);
        }
        if (auto forEachNode = dynamic_cast<ForEachNode*>(node))
        {
            return evaluateForEachNode(forEachNode);
        }
        if (auto breakNode = dynamic_cast<BreakNode*>(node))
        {
            return evaluateBreakNode(breakNode);
        }
        if (auto continueNode = dynamic_cast<ContinueNode*>(node))
        {
            return evaluateContinueNode(continueNode);
        }
        if (auto switchNode = dynamic_cast<SwitchNode*>(node))
        {
            return evaluateSwitchNode(switchNode);
        }
        if (auto caseNode = dynamic_cast<CaseNode*>(node))
        {
            return evaluateCaseNode(caseNode);
        }
        if (auto defaultNode = dynamic_cast<DefaultCaseNode*>(node))
        {
            return evaluateDefaultCaseNode(defaultNode);
        }
        if (auto importNode = dynamic_cast<ImportNode*>(node))
        {
            return evaluateImportNode(importNode);
        }
        if (auto funcNode = dynamic_cast<FunctionNode*>(node))
        {
            return evaluateFunctionNode(funcNode);
        }
        if (auto retNode = dynamic_cast<ReturnNode*>(node))
        {
            return evaluateReturnNode(retNode);
        }
        if (auto nativeNode = dynamic_cast<ast::nodes::statements::NativeFunctionNode*>(node))
        {
            return evaluateNativeFunctionNode(nativeNode);
        }

        return std::monostate{};
    }

    bool StatementEvaluator::canHandle(ASTNode* node) const
    {
        return isStatementNode(node);
    }

    bool StatementEvaluator::shouldBreakOrContinue() const
    {
        return flowManager->shouldBreakOrContinue();
    }

    void StatementEvaluator::handleBreak()
    {
        flowManager->handleBreak();
    }

    void StatementEvaluator::handleContinue()
    {
        flowManager->handleContinue();
    }

    void StatementEvaluator::enterLoop()
    {
        flowManager->enterLoop();
    }

    void StatementEvaluator::exitLoop()
    {
        flowManager->exitLoop();
    }

    bool StatementEvaluator::isInLoop() const
    {
        return flowManager->isInLoop();
    }

    void StatementEvaluator::enterBreakableContext()
    {
        flowManager->enterBreakableContext();
    }

    void StatementEvaluator::exitBreakableContext()
    {
        flowManager->exitBreakableContext();
    }

    bool StatementEvaluator::isInBreakableContext() const
    {
        return flowManager->isInBreakableContext();
    }

    void StatementEvaluator::resetLoopFlags()
    {
        flowManager->resetLoopFlags();
    }

    void StatementEvaluator::setExpressionEvaluator(ExpressionEvaluator* evaluator)
    {
        exprEvaluator = evaluator;
        loopEvaluator->setExpressionEvaluator(evaluator);
        declarationHandler->setExpressionEvaluator(evaluator);
        controlFlowHandler->setExpressionEvaluator(evaluator);
        importAndFunctionHandler->setExpressionEvaluator(evaluator);
    }

    void StatementEvaluator::setObjectEvaluator(ObjectEvaluator* evaluator)
    {
        objEvaluator = evaluator;
        loopEvaluator->setObjectEvaluator(evaluator);
        declarationHandler->setObjectEvaluator(evaluator);
    }

    bool StatementEvaluator::isStatementNode(ASTNode* node) const
    {
        return dynamic_cast<ProgramNode*>(node) ||
            dynamic_cast<BlockNode*>(node) ||
            dynamic_cast<DeclarationNode*>(node) ||
            dynamic_cast<AssignmentNode*>(node) ||
            dynamic_cast<IfNode*>(node) ||
            dynamic_cast<WhileNode*>(node) ||
            dynamic_cast<DoWhileNode*>(node) ||
            dynamic_cast<ForNode*>(node) ||
            dynamic_cast<ForEachNode*>(node) ||
            dynamic_cast<BreakNode*>(node) ||
            dynamic_cast<ContinueNode*>(node) ||
            dynamic_cast<SwitchNode*>(node) ||
            dynamic_cast<CaseNode*>(node) ||
            dynamic_cast<DefaultCaseNode*>(node) ||
            dynamic_cast<ImportNode*>(node) ||
            dynamic_cast<FunctionNode*>(node) ||
            dynamic_cast<ReturnNode*>(node) ||
            dynamic_cast<ast::nodes::statements::NativeFunctionNode*>(node);
    }

    Value StatementEvaluator::executeStatementList(const std::vector<std::unique_ptr<ASTNode>>& statements)
    {
        Value lastValue = std::monostate{};

        for (size_t i = 0; i < statements.size(); i++)
        {
            const auto& statement = statements[i];
            // Delegate evaluation based on statement type
            if (canHandle(statement.get()))
            {
                lastValue = evaluate(statement.get());
            }
            else if (exprEvaluator && exprEvaluator->canHandle(statement.get()))
            {
                lastValue = exprEvaluator->evaluate(statement.get());
            }
            else if (objEvaluator && objEvaluator->canHandle(statement.get()))
            {
                lastValue = objEvaluator->evaluate(statement.get());
            }

            // Check for control flow changes
            if (context->shouldReturn() || shouldBreakOrContinue())
            {
                break;
            }
        }

        return lastValue;
    }

    Value StatementEvaluator::evaluateProgramNode(ProgramNode* node)
    {
        return executeStatementList(node->getStatements());
    }

    Value StatementEvaluator::evaluateBlockNode(BlockNode* node)
    {
        auto env = context->getEnvironment();
        env->enterScope("", ScopeType::BLOCK);

        Value lastValue = std::monostate{};

        try
        {
            lastValue = executeStatementList(node->getStatements());
        }
        catch (...)
        {
            env->exitScope();
            throw;
        }

        env->exitScope();
        return lastValue;
    }

    Value StatementEvaluator::evaluateDeclarationNode(DeclarationNode* node)
    {
        return declarationHandler->evaluateDeclaration(node);
    }

    void StatementEvaluator::validateVariableDeclaration(DeclarationNode* node)
    {
        auto env = context->getEnvironment();

        // Check if variable already exists in current scope
        if (env->getScopeManager()->hasVariableInCurrentScope(node->getVariableName()))
        {
            throw EnvironmentException("Variable '" + node->getVariableName() +
                                       "' is already defined in this scope", node->getLocation());
        }
    }

    void StatementEvaluator::validateTypeAssignment(ValueType expectedType, const Value& value,
                                                    const std::string& variableName,
                                                    const SourceLocation& location)
    {
        // Delegate to TypeValidator utility
        validation::TypeValidator::validateAssignment(expectedType, value, variableName, location);
    }

    void StatementEvaluator::validateTypeAssignment(ValueType expectedType, const Value& value,
                                                    const std::string& variableName,
                                                    const SourceLocation& location,
                                                    const std::string& expectedClassName)
    {
        // Delegate to TypeValidator utility, passing context
        validation::TypeValidator::validateAssignment(expectedType, value, variableName, location, expectedClassName, context);
    }

    void StatementEvaluator::validateAssignmentAsDeclaration(AssignmentNode* node)
    {
        auto env = context->getEnvironment();

        // Check if variable already exists in current scope
        if (env->getScopeManager()->hasVariableInCurrentScope(node->getVariableName()))
        {
            throw EnvironmentException("Variable '" + node->getVariableName() +
                                       "' is already defined in this scope", node->getLocation());
        }
    }

    void StatementEvaluator::validateClassExists(const std::string& className, const SourceLocation& location)
    {
        // Delegate to TypeValidator utility
        validation::TypeValidator::validateClassExists(className, location, context);
    }

    bool StatementEvaluator::isValidTypeConversion(ValueType from, ValueType to)
    {
        // Delegate to TypeValidator utility
        return validation::TypeValidator::isValidTypeConversion(from, to);
    }

    bool StatementEvaluator::isGenericTypeCompatible(const std::string& actualClassName,
                                                     const std::string& expectedClassName)
    {
        // Delegate to TypeValidator utility
        return validation::TypeValidator::isGenericTypeCompatible(actualClassName, expectedClassName);
    }

    void StatementEvaluator::validateObjectTypeCompatibility(const Value& value,
                                                             const std::string& variableName,
                                                             const SourceLocation& location,
                                                             const std::string& expectedClassName)
    {
        // Delegate to TypeValidator utility
        validation::TypeValidator::validateObjectTypeCompatibility(value, variableName, location, expectedClassName, context);
    }

    Value StatementEvaluator::evaluateAssignmentNode(AssignmentNode* node)
    {
        return declarationHandler->evaluateAssignment(node);
    }

    Value StatementEvaluator::evaluateIfNode(IfNode* node)
    {
        return controlFlowHandler->evaluateIf(node);
    }

    Value StatementEvaluator::evaluateWhileNode(WhileNode* node)
    {
        return loopEvaluator->evaluateWhile(node);
    }

    Value StatementEvaluator::evaluateDoWhileNode(DoWhileNode* node)
    {
        return loopEvaluator->evaluateDoWhile(node);
    }

    Value StatementEvaluator::evaluateForNode(ForNode* node)
    {
        return loopEvaluator->evaluateFor(node);
    }

    Value StatementEvaluator::evaluateBreakNode(BreakNode* node)
    {
        if (!isInBreakableContext())
        {
            throw errors::RuntimeException("'break' statement must be inside a loop or switch statement",
                                           node->getLocation());
        }
        handleBreak();
        throw BreakException();
    }

    Value StatementEvaluator::evaluateContinueNode(ContinueNode* node)
    {
        if (!isInLoop())
        {
            throw errors::RuntimeException("'continue' statement must be inside a loop", node->getLocation());
        }
        handleContinue();
        throw ContinueException();
    }

    Value StatementEvaluator::evaluateReturnNode(ReturnNode* node)
    {
        Value returnValue = std::monostate{};

        if (node->getReturnValue() && exprEvaluator)
        {
            returnValue = exprEvaluator->evaluate(node->getReturnValue());

            // Note: Lambda-to-interface conversion for function returns should be handled
            // at the call site where we know the expected return type, not here.
            // For now, we leave lambda returns as-is and let the caller handle conversion.
        }

        context->pushReturnValue(returnValue);
        context->setReturned(true);

        throw ReturnException(returnValue);
    }

    // Switch statement implementation
    Value StatementEvaluator::evaluateSwitchNode(SwitchNode* node)
    {
        return controlFlowHandler->evaluateSwitch(node);
    }

    Value StatementEvaluator::evaluateCaseNode(CaseNode* node)
    {
        // Case nodes should not be evaluated directly - they are handled within switch statements
        throw TypeException("Case nodes should not be evaluated directly outside of switch statement");
    }

    Value StatementEvaluator::evaluateDefaultCaseNode(DefaultCaseNode* node)
    {
        // Default case nodes should not be evaluated directly - they are handled within switch statements
        throw TypeException("Default case nodes should not be evaluated directly outside of switch statement");
    }

    Value StatementEvaluator::evaluateImportNode(ImportNode* node)
    {
        return importAndFunctionHandler->evaluateImport(node);
    }

    Value StatementEvaluator::evaluateFunctionNode(FunctionNode* node)
    {
        return importAndFunctionHandler->evaluateFunction(node);
    }

    Value StatementEvaluator::evaluateNativeFunctionNode(ast::nodes::statements::NativeFunctionNode* node)
    {
        return importAndFunctionHandler->evaluateNativeFunction(node);
    }

    void StatementEvaluator::evaluateRecursively(ASTNode* node)
    {
        if (!node) return;

        // Special handling for ProgramNode - evaluate all its statements
        if (auto programNode = dynamic_cast<ast::nodes::statements::ProgramNode*>(node))
        {
            const auto& statements = programNode->getStatements();
            for (const auto& stmt : statements)
            {
                evaluateRecursively(stmt.get());
            }
            return;
        }

        // Handle different node types by delegating to appropriate evaluators
        if (canHandle(node))
        {
            // This is a statement node - evaluate it ourselves
            evaluate(node);
        }
        else if (exprEvaluator && exprEvaluator->canHandle(node))
        {
            // This is an expression node - delegate to expression evaluator
            exprEvaluator->evaluate(node);
        }
        else if (objEvaluator && objEvaluator->canHandle(node))
        {
            // This is an object node - delegate to object evaluator
            objEvaluator->evaluate(node);
        }
        else
        {
            // Unknown node type - this shouldn't happen in a well-formed AST
            throw TypeException("Unknown node type during import evaluation");
        }
    }

    value::Value StatementEvaluator::evaluateForEachNode(ForEachNode* node)
    {
        return loopEvaluator->evaluateForEach(node);
    }

    Value StatementEvaluator::convertLambdaToInterface(const Value& lambdaValue, const std::string& interfaceName, const SourceLocation& location)
    {
        return importAndFunctionHandler->convertLambdaToInterface(lambdaValue, interfaceName, location);
    }
}
