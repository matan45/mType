#include "StatementEvaluator.hpp"
#include "statements/LoopEvaluator.hpp"
#include "statements/DeclarationHandler.hpp"
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
          exprEvaluator(nullptr), objEvaluator(nullptr)
    {
        // Set back-reference so LoopEvaluator can call back to StatementEvaluator
        loopEvaluator->setStatementEvaluator(this);
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
        if (auto nativeNode = dynamic_cast<NativeFunctionNode*>(node))
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
            dynamic_cast<NativeFunctionNode*>(node);
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
        if (!exprEvaluator)
        {
            throw TypeException("Expression evaluator not available for if statement evaluation");
        }

        Value conditionValue = exprEvaluator->evaluate(node->getCondition());
        bool isTrue = exprEvaluator->isTruthy(conditionValue);

        if (isTrue)
        {
            return evaluate(node->getThenStatement());
        }
        else if (node->getElseStatement())
        {
            return evaluate(node->getElseStatement());
        }

        return std::monostate{};
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
        if (!exprEvaluator)
        {
            throw TypeException("Expression evaluator not available for switch statement evaluation");
        }

        // Evaluate the switch expression
        Value switchValue = exprEvaluator->evaluate(node->getExpression());

        Value lastValue = std::monostate{};
        bool foundMatch = false;
        bool executeRemaining = false; // For fallthrough behavior

        resetLoopFlags(); // Reset any existing break/continue flags

        // Enter breakable context for switch statement
        enterBreakableContext();

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
                        if (ValueConverter::compareValues(switchValue, caseValue))
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
                                lastValue = evaluate(statement.get());
                            }
                            catch (const BreakException&)
                            {
                                // Break encountered - exit switch
                                resetLoopFlags();
                                exitBreakableContext();
                                return lastValue;
                            }

                            // Check for other control flow interruptions
                            if (context->shouldReturn())
                            {
                                exitBreakableContext();
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
                                lastValue = evaluate(statement.get());
                            }
                            catch (const BreakException&)
                            {
                                // Break encountered - exit switch
                                resetLoopFlags();
                                exitBreakableContext();
                                return lastValue;
                            }

                            // Check for other control flow interruptions
                            if (context->shouldReturn())
                            {
                                exitBreakableContext();
                                return lastValue;
                            }
                        }

                        // Default case has implicit break - exit switch after executing default
                        exitBreakableContext();
                        return lastValue;
                    }
                }
            }
        }
        catch (const BreakException&)
        {
            // Break caught at switch level
            resetLoopFlags();
        }

        // Exit breakable context before returning
        exitBreakableContext();
        return lastValue;
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
        // Check if import is already resolved (e.g., from JSON deserialization)
        if (node->isResolved() && node->getImportedAST())
        {
            // Directly evaluate the embedded AST instead of resolving from files
            ASTNode* importedAST = node->getImportedAST();

            // Evaluate the embedded AST directly using the current evaluation context
            // This should register all classes, functions, etc. from the import
            if (importedAST)
            {
                // Recursively evaluate the imported AST using the statement evaluator
                Value result = this->evaluate(importedAST);
            }

            return std::monostate{}; // Import completed successfully
        }

        // Fall back to normal ImportManager-based resolution for non-resolved imports
        auto env = context->getEnvironment();
        auto importManager = env->getImportManager();

        if (!importManager)
        {
            throw TypeException("Import manager not available for import evaluation");
        }

        std::string filePath = node->getFilePath();

        // Determine execution mode based on file extension
        //bool isCacheMode = filePath.ends_with(".mtc");

        // For cache mode with release directory, keep .mtc paths for both tracking and resolution
        // For normal mode, use .mt paths for tracking consistency
        //std::string trackingPath = filePath;
        //std::string resolutionPath = filePath;

        /* if (!isCacheMode && filePath.ends_with(".mtc")) {
             trackingPath = filePath.substr(0, filePath.length() - 1); // Remove 'c' to get .mt
             resolutionPath = trackingPath;
         } else if (isCacheMode && !filePath.ends_with(".mtc")) {
             // Convert .mt to .mtc for cache mode
             resolutionPath = filePath + "c";
             trackingPath = resolutionPath;
         }*/

        // Use appropriate path for resolution based on execution mode
        std::string resolvedPath = importManager->resolvePath(filePath);

        // Check if already evaluated to avoid re-evaluation
        if (importManager->isEvaluated(resolvedPath))
        {
            return std::monostate{}; // Already imported
        }

        // Check for circular imports
        if (importManager->isBeingEvaluated(resolvedPath))
        {
            throw TypeException("Circular import detected: " + filePath + " is already being imported");
        }

        // Mark as being evaluated to prevent circular imports
        importManager->markAsBeingEvaluated(resolvedPath);

        try
        {
            // Normal mode: Parse .mt file
            ASTNode* importedAST = importManager->parseAndCacheAST(filePath);

            // Set import evaluation context and evaluate the loaded AST
            env->setImportEvaluation(true);
            evaluateRecursively(importedAST);
            env->setImportEvaluation(false);

            // Mark as evaluated and no longer being evaluated
            importManager->markAsEvaluated(resolvedPath);
            importManager->unmarkAsBeingEvaluated(resolvedPath);

            return std::monostate{}; // Imports return void
        }
        catch (...)
        {
            env->setImportEvaluation(false);
            importManager->unmarkAsBeingEvaluated(resolvedPath);
            throw;
        }
    }

    Value StatementEvaluator::evaluateFunctionNode(FunctionNode* node)
    {
        auto env = context->getEnvironment();

        // Create function definition with interface support
        auto genericRetType = node->getGenericReturnType();
        std::string returnClassName = "";

        if (genericRetType && node->getReturnType() == ValueType::OBJECT)
        {
            // For object types, try to get the specific class/interface name
            std::string typeName = genericRetType->getBaseTypeName();
            if (typeName != "object")
            {
                returnClassName = typeName;
            }
            else if (genericRetType->isGenericParameter())
            {
                // It's a generic parameter, get its name
                returnClassName = genericRetType->getGenericName();
            }
        }

        // Use backward compatibility constructor, then set return class name
        auto funcDef = std::make_shared<FunctionDefinition>(
            node->getName(),
            node->getReturnType(),
            node->getParameters()
        );

        // Set the return class name if we found one
        if (!returnClassName.empty())
        {
            funcDef->setReturnClassName(returnClassName);
        }

        // Set the function body
        funcDef->setBody(node->getBody());

        // Register function in environment
        env->registerFunction(node->getName(), funcDef);


        return std::monostate{}; // Function definitions don't return values
    }

    Value StatementEvaluator::evaluateNativeFunctionNode(NativeFunctionNode* node)
    {
        throw TypeException("Native function evaluation not implemented in refactored version");
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
        // Extract the lambda value
        auto lambdaPtr = std::get<std::shared_ptr<value::LambdaValue>>(lambdaValue);
        auto* lambdaNode = lambdaPtr->getLambda();

        // Get the interface definition from the environment
        auto env = context->getEnvironment();
        auto interfaceDef = env->findInterface(interfaceName);

        if (!interfaceDef)
        {
            return lambdaValue;
        }

        // Check if the interface is functional (has exactly one method)
        if (!interfaceDef->isFunctionalInterface())
        {
            auto methodSignatures = interfaceDef->getMethodSignatures();
            throw errors::TypeException(
                "Cannot assign lambda to non-functional interface '" + interfaceName + "'. " +
                "Lambdas can only be assigned to interfaces with exactly one method. " +
                "Interface '" + interfaceName + "' has " + std::to_string(methodSignatures.size()) + " methods. " +
                "Consider using a functional interface (single method) or implement the interface explicitly.",
                location
            );
        }

        // Create the lambda implementation class
        auto implClass = interfaceDef->createLambdaImplementation(lambdaNode);
        if (!implClass)
        {
            return lambdaValue;
        }

        // Create an instance of the implementation class
        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(implClass);

        // Store the lambda in a special field that the implementation can access
        instance->setField(constants::lambda::LAMBDA_FIELD_NAME, lambdaValue);

        return instance;
    }
}
