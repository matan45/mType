#include "StatementEvaluator.hpp"
#include "../services/ImportManager.hpp"
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
#include "../errors/EnvironmentException.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include "../ast/nodes/statements/ContinueNode.hpp"
#include "ExpressionEvaluator.hpp"
#include "ObjectEvaluator.hpp"

namespace evaluator
{
    using namespace errors;
    using namespace exception;
    using namespace runtimeTypes::global;
    using namespace environment::manager;
    using namespace services;
    
    StatementEvaluator::StatementEvaluator(std::shared_ptr<EvaluationContext> ctx)
        : context(ctx), flowManager(std::make_unique<ControlFlowManager>()),
          exprEvaluator(nullptr), objEvaluator(nullptr)
    {
    }
    
    Value StatementEvaluator::evaluate(ASTNode* node)
    {
        if (!node) {
            return std::monostate{};
        }
        
        // Try to handle with this evaluator first
        if (!canHandle(node)) {
            // Delegate to other evaluators if this one can't handle it
            if (exprEvaluator && exprEvaluator->canHandle(node)) {
                return exprEvaluator->evaluate(node);
            } else if (objEvaluator && objEvaluator->canHandle(node)) {
                return objEvaluator->evaluate(node);
            }
            return std::monostate{};
        }
        
        // Dispatch to appropriate evaluation method based on node type
        if (auto progNode = dynamic_cast<ProgramNode*>(node)) {
            return evaluateProgramNode(progNode);
        }
        if (auto blockNode = dynamic_cast<BlockNode*>(node)) {
            return evaluateBlockNode(blockNode);
        }
        if (auto declNode = dynamic_cast<DeclarationNode*>(node)) {
            return evaluateDeclarationNode(declNode);
        }
        if (auto assignNode = dynamic_cast<AssignmentNode*>(node)) {
            return evaluateAssignmentNode(assignNode);
        }
        if (auto ifNode = dynamic_cast<IfNode*>(node)) {
            return evaluateIfNode(ifNode);
        }
        if (auto whileNode = dynamic_cast<WhileNode*>(node)) {
            return evaluateWhileNode(whileNode);
        }
        if (auto doWhileNode = dynamic_cast<DoWhileNode*>(node)) {
            return evaluateDoWhileNode(doWhileNode);
        }
        if (auto forNode = dynamic_cast<ForNode*>(node)) {
            return evaluateForNode(forNode);
        }
        if (auto breakNode = dynamic_cast<BreakNode*>(node)) {
            return evaluateBreakNode(breakNode);
        }
        if (auto continueNode = dynamic_cast<ContinueNode*>(node)) {
            return evaluateContinueNode(continueNode);
        }
        if (auto switchNode = dynamic_cast<SwitchNode*>(node)) {
            return evaluateSwitchNode(switchNode);
        }
        if (auto caseNode = dynamic_cast<CaseNode*>(node)) {
            return evaluateCaseNode(caseNode);
        }
        if (auto defaultNode = dynamic_cast<DefaultCaseNode*>(node)) {
            return evaluateDefaultCaseNode(defaultNode);
        }
        if (auto importNode = dynamic_cast<ImportNode*>(node)) {
            return evaluateImportNode(importNode);
        }
        if (auto funcNode = dynamic_cast<FunctionNode*>(node)) {
            return evaluateFunctionNode(funcNode);
        }
        if (auto retNode = dynamic_cast<ReturnNode*>(node)) {
            return evaluateReturnNode(retNode);
        }
        if (auto nativeNode = dynamic_cast<NativeFunctionNode*>(node)) {
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
    
    void StatementEvaluator::resetLoopFlags()
    {
        flowManager->resetLoopFlags();
    }
    
    void StatementEvaluator::setExpressionEvaluator(ExpressionEvaluator* evaluator)
    {
        exprEvaluator = evaluator;
    }
    
    void StatementEvaluator::setObjectEvaluator(ObjectEvaluator* evaluator)
    {
        objEvaluator = evaluator;
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
        
        for (const auto& statement : statements) {
            // Delegate evaluation based on statement type
            if (canHandle(statement.get())) {
                lastValue = evaluate(statement.get());
            } else if (exprEvaluator && exprEvaluator->canHandle(statement.get())) {
                lastValue = exprEvaluator->evaluate(statement.get());
            } else if (objEvaluator && objEvaluator->canHandle(statement.get())) {
                lastValue = objEvaluator->evaluate(statement.get());
            }
            
            // Check for control flow changes
            if (context->shouldReturn() || shouldBreakOrContinue()) {
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
        
        try {
            lastValue = executeStatementList(node->getStatements());
        }
        catch (...) {
            env->exitScope();
            throw;
        }
        
        env->exitScope();
        return lastValue;
    }
    
    Value StatementEvaluator::evaluateDeclarationNode(DeclarationNode* node)
    {
        validateVariableDeclaration(node);
        
        auto env = context->getEnvironment();
        
        // Evaluate initial value if present, otherwise use default
        Value initialValue = std::monostate{};
        if (node->getInitializer() && exprEvaluator) {
            initialValue = exprEvaluator->evaluate(node->getInitializer());
            
            // Validate type compatibility between declared type and initial value
            validateTypeAssignment(node->getType(), initialValue, node->getVariableName(), node->getLocation());
        }
        
        // Create variable definition
        auto varDef = std::make_shared<VariableDefinition>(
            node->getVariableName(), 
            node->getType(), 
            initialValue,
            node->isFinal()
        );
        
        env->declareVariable(node->getVariableName(), varDef);
        return initialValue;
    }
    
    void StatementEvaluator::validateVariableDeclaration(DeclarationNode* node)
    {
        auto env = context->getEnvironment();
        
        // Check if variable already exists in current scope
        if (env->getScopeManager()->hasVariableInCurrentScope(node->getVariableName())) {
            throw EnvironmentException("Variable '" + node->getVariableName() + 
                                     "' is already defined in this scope", node->getLocation());
        }
    }
    
    void StatementEvaluator::validateTypeAssignment(ValueType expectedType, const Value& value, 
                                                   const std::string& variableName, 
                                                   const SourceLocation& location)
    {
        // Skip validation for void initializers (default values)
        if (std::holds_alternative<std::monostate>(value) && expectedType != ValueType::VOID) {
            return; // Allow default initialization
        }
        
        ValueType actualType = utils::ValueConverter::getValueType(value);
        
        // Allow null assignment to object types
        if (actualType == ValueType::NULL_TYPE && expectedType == ValueType::OBJECT) {
            return;
        }
        
        // Allow exact type matches
        if (actualType == expectedType) {
            return;
        }
        
        // Allow valid implicit conversions
        if (isValidTypeConversion(actualType, expectedType)) {
            return;
        }
        
        // Type mismatch
        throw TypeException("Type mismatch for variable '" + variableName + "': expected " + 
                          utils::ValueConverter::valueTypeToString(expectedType) + 
                          " but got " + utils::ValueConverter::valueTypeToString(actualType), 
                          location);
    }
    
    void StatementEvaluator::validateTypeAssignment(ValueType expectedType, const Value& value, 
                                                   const std::string& variableName, 
                                                   const SourceLocation& location,
                                                   const std::string& expectedClassName)
    {
        // Skip validation for void initializers (default values)
        if (std::holds_alternative<std::monostate>(value) && expectedType != ValueType::VOID) {
            return; // Allow default initialization
        }
        
        ValueType actualType = utils::ValueConverter::getValueType(value);
        
        // Allow null assignment to object types
        if (actualType == ValueType::NULL_TYPE && expectedType == ValueType::OBJECT) {
            return;
        }
        
        // Allow exact type matches
        if (actualType == expectedType) {
            // For object types, validate class compatibility
            if (actualType == ValueType::OBJECT && expectedType == ValueType::OBJECT && !expectedClassName.empty()) {
                validateObjectTypeCompatibility(value, variableName, location, expectedClassName);
            }
            return;
        }
        
        // Allow valid implicit conversions
        if (isValidTypeConversion(actualType, expectedType)) {
            return;
        }
        
        // Type mismatch
        throw TypeException("Type mismatch for variable '" + variableName + "': expected " + 
                          utils::ValueConverter::valueTypeToString(expectedType) + 
                          " but got " + utils::ValueConverter::valueTypeToString(actualType), 
                          location);
    }
    
    void StatementEvaluator::validateAssignmentAsDeclaration(AssignmentNode* node)
    {
        auto env = context->getEnvironment();
        
        // Check if variable already exists in current scope
        if (env->getScopeManager()->hasVariableInCurrentScope(node->getVariableName())) {
            throw EnvironmentException("Variable '" + node->getVariableName() + 
                                     "' is already defined in this scope", node->getLocation());
        }
    }
    
    void StatementEvaluator::validateClassExists(const std::string& className, const SourceLocation& location)
    {
        auto env = context->getEnvironment();
        
        // Check if the class is defined
        if (!env->findClass(className)) {
            throw UndefinedException("Class '" + className + "' is not defined", location);
        }
    }
    
    bool StatementEvaluator::isValidTypeConversion(ValueType from, ValueType to)
    {
        // Allow int to float conversion (common implicit conversion)
        if (from == ValueType::INT && to == ValueType::FLOAT) {
            return true;
        }
        
        // Add other valid conversions here if needed
        // For example, you might allow:
        // - bool to int (false = 0, true = 1)
        // - char to int
        // etc.
        
        return false; // No other conversions allowed for now
    }
    
    void StatementEvaluator::validateObjectTypeCompatibility(const Value& value, 
                                                           const std::string& variableName, 
                                                           const SourceLocation& location)
    {
        // This is the legacy version without expected class name
        // For now, we'll skip validation since we don't have the expected class
        // This should be used only when class name information is not available
    }
    
    void StatementEvaluator::validateObjectTypeCompatibility(const Value& value, 
                                                           const std::string& variableName, 
                                                           const SourceLocation& location,
                                                           const std::string& expectedClassName)
    {
        // Extract object instance and get its class name
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(value)) {
            auto objInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(value);
            if (objInstance) {
                std::string actualClassName = objInstance->getClassDefinition()->getName();
                
                // Check if the actual class matches the expected class
                if (actualClassName != expectedClassName) {
                    throw TypeException("Object type mismatch for variable '" + variableName + "': expected " + 
                                      expectedClassName + " but got " + actualClassName, location);
                }
                
                // TODO: In a full implementation, this would also check class hierarchies
                // to allow assignments like Derived -> Base (inheritance compatibility)
            }
        }
    }
    
    Value StatementEvaluator::evaluateAssignmentNode(AssignmentNode* node)
    {
        if (!exprEvaluator) {
            throw TypeException("Expression evaluator not available for assignment evaluation");
        }
        
        // Evaluate the new value
        Value newValue = exprEvaluator->evaluate(node->getValue());
        
        auto env = context->getEnvironment();
        
        // PRIORITY: Check for implicit field assignment first when in a constructor/instance context
        // This ensures field assignments work correctly even when there are shadowing parameters
        auto currentInstance = context->getCurrentInstance();
        if (currentInstance && node->getVariableType() == ValueType::VOID) {
            auto field = currentInstance->getField(node->getVariableName());
            if (field) {
                // This is an implicit field assignment - prioritize over variable shadowing
                currentInstance->setField(node->getVariableName(), newValue);
                return newValue;
            }
        }
        
        auto varDef = env->findVariable(node->getVariableName());
        
        if (!varDef) {
            // Check if this is a variable declaration (has type != VOID)
            if (node->getVariableType() != ValueType::VOID) {
                // This is a declaration - create the variable
                validateAssignmentAsDeclaration(node);
                
                // Validate that the class exists for object types
                if (node->getVariableType() == ValueType::OBJECT && !node->getClassName().empty()) {
                    validateClassExists(node->getClassName(), node->getLocation());
                }
                
                // Validate type compatibility for new variable declarations
                if (node->getVariableType() == ValueType::OBJECT && !node->getClassName().empty()) {
                    validateTypeAssignment(node->getVariableType(), newValue, node->getVariableName(), 
                                         node->getLocation(), node->getClassName());
                } else {
                    validateTypeAssignment(node->getVariableType(), newValue, node->getVariableName(), node->getLocation());
                }
                
                // Create variable definition with class name for object types
                auto newVarDef = std::make_shared<VariableDefinition>(
                    node->getVariableName(),
                    node->getVariableType(),
                    newValue,
                    node->getIsFinal(),
                    (node->getVariableType() == ValueType::OBJECT && !node->getClassName().empty()) 
                        ? node->getClassName() : ""
                );
                
                env->declareVariable(node->getVariableName(), newVarDef);
                return newValue;
            } else {
                // Check if this might be a static field assignment
                auto classRegistry = env->getClassRegistry();
                
                // Check if we have a current class name stored (from method execution)
                auto currentClassVar = env->findVariable("__current_class_name__");
                if (currentClassVar) {
                    auto currentClassValue = currentClassVar->getValue();
                    if (std::holds_alternative<std::string>(currentClassValue)) {
                        std::string className = std::get<std::string>(currentClassValue);
                        auto classDef = env->findClass(className);
                        if (classDef) {
                            auto field = classDef->getField(node->getVariableName());
                            if (field && field->isStatic()) {
                                // This is a static field assignment
                                field->setValue(newValue);
                                return newValue;
                            }
                        }
                    }
                }
                
                // This is a pure assignment to non-existent variable
                throw UndefinedException("Variable '" + node->getVariableName() + "' is not defined", 
                                       node->getLocation());
            }
        }
        
        // Check if this is an attempt to redeclare an existing variable
        if (node->getVariableType() != ValueType::VOID) {
            // Check if variable exists in current scope - this is always an error
            if (env->getScopeManager()->hasVariableInCurrentScope(node->getVariableName())) {
                throw EnvironmentException("Variable '" + node->getVariableName() + 
                                         "' is already defined in this scope", node->getLocation());
            }
            
            // Check if we're trying to redeclare a function parameter within the same function
            if (varDef && env->getScopeManager()->isInFunction()) {
                // We're in a function and the variable exists - check if it's a function parameter
                // by checking if it exists in the immediate parent function scope
                auto currentScope = env->getScopeManager()->getCurrentScope();
                if (currentScope && currentScope->getParent() && 
                    currentScope->getParent()->getType() == ScopeType::FUNCTION) {
                    // Check if the variable is defined in the function scope (i.e., it's a parameter)
                    if (currentScope->getParent()->hasVariableInCurrentScope(node->getVariableName())) {
                        throw EnvironmentException("Variable '" + node->getVariableName() + 
                                                 "' is already defined in this scope", node->getLocation());
                    }
                }
                
                // If we reach here, it's valid scope shadowing (e.g., global -> method)
                // Create new variable definition with the specified type
                ValueType declaredType = node->getVariableType();
                
                // Validate type compatibility
                if (declaredType == ValueType::OBJECT && !node->getClassName().empty()) {
                    validateTypeAssignment(declaredType, newValue, node->getVariableName(), 
                                         node->getLocation(), node->getClassName());
                } else {
                    validateTypeAssignment(declaredType, newValue, node->getVariableName(), node->getLocation());
                }
                
                // Create new variable definition with class name for object types
                auto newVarDef = std::make_shared<VariableDefinition>(
                    node->getVariableName(),
                    declaredType,
                    newValue,
                    node->getIsFinal(),
                    (declaredType == ValueType::OBJECT && !node->getClassName().empty()) 
                        ? node->getClassName() : ""
                );
                
                env->declareVariable(node->getVariableName(), newVarDef);
                return newValue;
            }
            
            // Variable exists but we're not in a function, or variable doesn't exist in parent scope
            if (varDef) {
                // This is valid scope shadowing (e.g., global -> method)
                // Create new variable definition with the specified type
                ValueType declaredType = node->getVariableType();
                
                // Validate type compatibility
                if (declaredType == ValueType::OBJECT && !node->getClassName().empty()) {
                    validateTypeAssignment(declaredType, newValue, node->getVariableName(), 
                                         node->getLocation(), node->getClassName());
                } else {
                    validateTypeAssignment(declaredType, newValue, node->getVariableName(), node->getLocation());
                }
                
                // Create new variable definition with class name for object types
                auto newVarDef = std::make_shared<VariableDefinition>(
                    node->getVariableName(),
                    declaredType,
                    newValue,
                    node->getIsFinal(),
                    (declaredType == ValueType::OBJECT && !node->getClassName().empty()) 
                        ? node->getClassName() : ""
                );
                
                env->declareVariable(node->getVariableName(), newVarDef);
                return newValue;
            }
        }
        
        // Existing variable assignment
        if (varDef->isFinal()) {
            throw TypeException("Cannot modify final variable '" + node->getVariableName() + "'", 
                              node->getLocation());
        }
        
        // Validate type compatibility for existing variable assignments
        if (varDef->getType() == ValueType::OBJECT && !varDef->getClassName().empty()) {
            validateTypeAssignment(varDef->getType(), newValue, node->getVariableName(), 
                                 node->getLocation(), varDef->getClassName());
        } else {
            validateTypeAssignment(varDef->getType(), newValue, node->getVariableName(), node->getLocation());
        }
        
        varDef->setValue(newValue);
        return newValue;
    }
    
    Value StatementEvaluator::evaluateIfNode(IfNode* node)
    {
        if (!exprEvaluator) {
            throw TypeException("Expression evaluator not available for if statement evaluation");
        }
        
        Value conditionValue = exprEvaluator->evaluate(node->getCondition());
        bool isTrue = exprEvaluator->isTruthy(conditionValue);
        
        if (isTrue) {
            return evaluate(node->getThenStatement());
        } else if (node->getElseStatement()) {
            return evaluate(node->getElseStatement());
        }
        
        return std::monostate{};
    }
    
    Value StatementEvaluator::evaluateWhileNode(WhileNode* node)
    {
        if (!exprEvaluator) {
            throw TypeException("Expression evaluator not available for while loop evaluation");
        }
        
        Value lastValue = std::monostate{};
        resetLoopFlags();
        
        try {
            while (true) {
                Value conditionValue = exprEvaluator->evaluate(node->getCondition());
                if (!exprEvaluator->isTruthy(conditionValue)) {
                    break;
                }
                
                try {
                    lastValue = evaluate(node->getBody());
                } catch (const ContinueException&) {
                    // Continue caught - reset flags and continue to next iteration
                    resetLoopFlags();
                    continue; // Continue the while loop
                }
                
                if (context->shouldReturn() || flowManager->isBreaking()) {
                    break;
                }
                
                if (flowManager->isContinuing()) {
                    resetLoopFlags();
                    continue;
                }
            }
        }
        catch (const BreakException&) {
            // Break caught, exit loop normally
        }
        
        resetLoopFlags();
        return lastValue;
    }
    
    Value StatementEvaluator::evaluateDoWhileNode(DoWhileNode* node)
    {
        if (!exprEvaluator) {
            throw TypeException("Expression evaluator not available for do-while loop evaluation");
        }
        
        Value lastValue = std::monostate{};
        resetLoopFlags();
        
        try {
            do {
                try {
                    lastValue = evaluate(node->getBody());
                } catch (const ContinueException&) {
                    // Continue caught - reset flags and continue to condition check
                    resetLoopFlags();
                    // Fall through to condition check
                }
                
                if (context->shouldReturn() || flowManager->isBreaking()) {
                    break;
                }
                
                if (flowManager->isContinuing()) {
                    resetLoopFlags();
                    // Check condition before next iteration
                }
                
                Value conditionValue = exprEvaluator->evaluate(node->getCondition());
                if (!exprEvaluator->isTruthy(conditionValue)) {
                    break;
                }
                
            } while (true);
        }
        catch (const BreakException&) {
            // Break caught, exit loop normally
        }
        
        resetLoopFlags();
        return lastValue;
    }
    
    Value StatementEvaluator::evaluateForNode(ForNode* node)
    {
        if (!exprEvaluator) {
            throw TypeException("Expression evaluator not available for for loop evaluation");
        }
        
        auto env = context->getEnvironment();
        env->enterScope("", ScopeType::BLOCK);
        
        Value lastValue = std::monostate{};
        resetLoopFlags();
        
        try {
            // Initialize
            if (node->getInitialization()) {
                evaluate(node->getInitialization());
            }
            
            // Loop
            while (true) {
                // Check condition
                if (node->getCondition()) {
                    Value conditionValue = exprEvaluator->evaluate(node->getCondition());
                    if (!exprEvaluator->isTruthy(conditionValue)) {
                        break;
                    }
                }
                
                // Execute body
                try {
                    lastValue = evaluate(node->getBody());
                } catch (const ContinueException&) {
                    // Continue caught - reset flags and continue to update/next iteration
                    resetLoopFlags();
                    // Fall through to update
                }
                
                if (context->shouldReturn() || flowManager->isBreaking()) {
                    break;
                }
                
                if (flowManager->isContinuing()) {
                    resetLoopFlags();
                }
                
                // Update
                if (node->getUpdate()) {
                    evaluate(node->getUpdate());
                }
            }
        }
        catch (const BreakException&) {
            // Break caught, exit loop normally
        }
        catch (...) {
            env->exitScope();
            throw;
        }
        
        env->exitScope();
        resetLoopFlags();
        return lastValue;
    }
    
    Value StatementEvaluator::evaluateBreakNode(BreakNode* node)
    {
        handleBreak();
        throw BreakException();
    }
    
    Value StatementEvaluator::evaluateContinueNode(ContinueNode* node)
    {
        handleContinue();
        throw ContinueException();
    }
    
    Value StatementEvaluator::evaluateReturnNode(ReturnNode* node)
    {
        Value returnValue = std::monostate{};
        
        if (node->getReturnValue() && exprEvaluator) {
            returnValue = exprEvaluator->evaluate(node->getReturnValue());
        }
        
        context->pushReturnValue(returnValue);
        context->setReturned(true);
        
        throw ReturnException(returnValue);
    }
    
    // Switch statement implementation
    Value StatementEvaluator::evaluateSwitchNode(SwitchNode* node)
    {
        if (!exprEvaluator) {
            throw TypeException("Expression evaluator not available for switch statement evaluation");
        }
        
        // Evaluate the switch expression
        Value switchValue = exprEvaluator->evaluate(node->getExpression());
        
        Value lastValue = std::monostate{};
        bool foundMatch = false;
        bool executeRemaining = false; // For fallthrough behavior
        
        resetLoopFlags(); // Reset any existing break/continue flags
        
        try {
            // Iterate through all cases
            for (const auto& casePtr : node->getCases()) {
                if (auto caseNode = dynamic_cast<CaseNode*>(casePtr.get())) {
                    // Regular case
                    if (!foundMatch && !executeRemaining) {
                        // Check if case value matches switch value
                        Value caseValue = exprEvaluator->evaluate(caseNode->getValue());
                        if (ValueConverter::compareValues(switchValue, caseValue)) {
                            foundMatch = true;
                            executeRemaining = true;
                        }
                    }
                    
                    // Execute case statements if we're in fallthrough mode
                    if (executeRemaining) {
                        for (const auto& statement : caseNode->getStatements()) {
                            try {
                                lastValue = evaluate(statement.get());
                            } catch (const BreakException&) {
                                // Break encountered - exit switch
                                resetLoopFlags();
                                return lastValue;
                            }
                            
                            // Check for other control flow interruptions
                            if (context->shouldReturn()) {
                                return lastValue;
                            }
                        }
                    }
                } else if (auto defaultNode = dynamic_cast<DefaultCaseNode*>(casePtr.get())) {
                    // Default case - execute if no match found or if we're in fallthrough
                    if (!foundMatch || executeRemaining) {
                        foundMatch = true; // Mark as found for fallthrough logic
                        executeRemaining = true;
                        
                        for (const auto& statement : defaultNode->getStatements()) {
                            try {
                                lastValue = evaluate(statement.get());
                            } catch (const BreakException&) {
                                // Break encountered - exit switch
                                resetLoopFlags();
                                return lastValue;
                            }
                            
                            // Check for other control flow interruptions
                            if (context->shouldReturn()) {
                                return lastValue;
                            }
                        }
                        
                        // Default case has implicit break - exit switch after executing default
                        return lastValue;
                    }
                }
            }
        }
        catch (const BreakException&) {
            // Break caught at switch level
            resetLoopFlags();
        }
        
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
        auto env = context->getEnvironment();
        auto importManager = env->getImportManager();
        
        if (!importManager) {
            throw TypeException("Import manager not available for import evaluation");
        }
        
        std::string filePath = node->getFilePath();
        
        // Check if already evaluated to avoid re-evaluation
        if (importManager->isEvaluated(filePath)) {
            return std::monostate{}; // Already imported
        }
        
        // Check for circular imports
        if (importManager->isBeingEvaluated(filePath)) {
            throw TypeException("Circular import detected: " + filePath + " is already being imported");
        }
        
        try {
            // Parse and cache the AST (doesn't evaluate)
            ASTNode* importedAST = importManager->parseAndCacheAST(filePath);
            
            if (!importedAST) {
                throw TypeException("Failed to parse imported file: " + filePath);
            }
            
            // Mark as being evaluated to prevent circular imports
            importManager->markAsBeingEvaluated(filePath);
            
            // Set import evaluation context
            env->setImportEvaluation(true);
            
            try {
                // Evaluate the imported AST in the current environment
                // We need to recursively evaluate the AST using the appropriate evaluators
                // Since we can't access the coordinator directly, we'll evaluate it ourselves
                evaluateRecursively(importedAST);
                
                // Reset import evaluation context
                env->setImportEvaluation(false);
                
                // Mark as evaluated and no longer being evaluated
                importManager->markAsEvaluated(filePath);
                importManager->unmarkAsBeingEvaluated(filePath);
                
                return std::monostate{}; // Imports return void
                
            } catch (...) {
                env->setImportEvaluation(false);
                importManager->unmarkAsBeingEvaluated(filePath);
                throw;
            }
            
        } catch (const std::exception& e) {
            throw TypeException("Error importing file '" + filePath + "': " + e.what());
        }
    }
    
    Value StatementEvaluator::evaluateFunctionNode(FunctionNode* node)
    {
        auto env = context->getEnvironment();
        
        // Create function definition
        auto funcDef = std::make_shared<FunctionDefinition>(
            node->getName(),
            node->getReturnType(),
            node->getParameters()
        );
        
        // Set the function body
        funcDef->setBody(node->getBody());
        
        // Register function in environment
        env->registerFunction(node->getName(), funcDef);
        
        return std::monostate{}; // Function definitions don't return values
    }
    
    Value StatementEvaluator::evaluateNativeFunctionNode(NativeFunctionNode* node)
    {
        // TODO: Implement native function evaluation
        throw TypeException("Native function evaluation not implemented in refactored version");
    }
    
    void StatementEvaluator::evaluateRecursively(ASTNode* node)
    {
        if (!node) return;
        
        // Special handling for ProgramNode - evaluate all its statements
        if (auto programNode = dynamic_cast<ast::nodes::statements::ProgramNode*>(node)) {
            const auto& statements = programNode->getStatements();
            for (const auto& stmt : statements) {
                evaluateRecursively(stmt.get());
            }
            return;
        }
        
        // Handle different node types by delegating to appropriate evaluators
        if (canHandle(node)) {
            // This is a statement node - evaluate it ourselves
            evaluate(node);
        } else if (exprEvaluator && exprEvaluator->canHandle(node)) {
            // This is an expression node - delegate to expression evaluator
            exprEvaluator->evaluate(node);
        } else if (objEvaluator && objEvaluator->canHandle(node)) {
            // This is an object node - delegate to object evaluator
            objEvaluator->evaluate(node);
        } else {
            // Unknown node type - this shouldn't happen in a well-formed AST
            throw TypeException("Unknown node type during import evaluation");
        }
    }
}