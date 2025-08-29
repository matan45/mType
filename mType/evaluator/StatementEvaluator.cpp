#include "StatementEvaluator.hpp"
#include "Evaluator.hpp"
#include "ExpressionEvaluator.hpp"
#include "../services/ImportManager.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../ast/nodes/statements/DeclarationNode.hpp"
#include "../ast/nodes/statements/CaseNode.hpp"
#include "../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../environment/manager/Scope.hpp"
#include "../exception/BreakException.hpp"
#include "../exception/ContinueException.hpp"
#include "../exception/ReturnException.hpp"
#include "../errors/TypeException.hpp"
#include "../errors/UndefinedException.hpp"
#include "../errors/ArgumentException.hpp"
#include "../errors/EnvironmentException.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../services/ImportManager.hpp"
#include <iostream>

namespace evaluator
{
    using namespace errors;
    using namespace exception;
    using namespace runtimeTypes::global;
    using namespace environment::manager;
    using namespace services;

    StatementEvaluator::StatementEvaluator(Evaluator* evaluator)
        : mainEvaluator(evaluator), breakFlag(false), continueFlag(false)
    {
    }

    Value StatementEvaluator::evaluateProgramNode(ProgramNode* node)
    {
        Value lastValue = std::monostate{};
        
        for (const auto& statement : node->getStatements()) {
            lastValue = mainEvaluator->evaluate(statement.get());
            
            // Only break on return if we're at global scope (not inside a function)
            if (mainEvaluator->shouldReturn() && !mainEvaluator->getEnvironment()->isInFunction()) {
                break;
            }
            
            // Reset return state after each program-level statement to prevent 
            // method returns from affecting subsequent statements
            mainEvaluator->setReturned(false);
        }
        
        return lastValue;
    }

    Value StatementEvaluator::evaluateBlockNode(BlockNode* node)
    {
        auto env = mainEvaluator->getEnvironment();
        env->enterScope("", ScopeType::BLOCK);
        
        Value lastValue = std::monostate{};
        
        try {
            for (const auto& statement : node->getStatements()) {
                lastValue = mainEvaluator->evaluate(statement.get());
                
                // Only break on return if we're inside a function
                // Break/continue should only affect loop execution, not block-level statement processing
                if (mainEvaluator->shouldReturn() && mainEvaluator->getEnvironment()->isInFunction()) {
                    break;
                }
            }
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
        auto env = mainEvaluator->getEnvironment();
        
        // Check if variable already exists in current scope (but allow redefinitions during import evaluation)
        if (!env->isEvaluatingImport() && env->getScopeManager()->hasVariableInCurrentScope(node->getVariableName())) {
            throw EnvironmentException("Variable '" + node->getVariableName() + "' is already defined in this scope", node->getLocation());
        }
        
        // Also check for parameter shadowing - variables can't shadow function parameters
        if (!env->isEvaluatingImport() && env->isInFunction()) {
            auto currentScope = env->getScopeManager()->getCurrentScope();
            // Walk up to find the function scope and check for parameter conflicts
            while (currentScope && currentScope->getType() != ScopeType::FUNCTION) {
                currentScope = currentScope->getParent();
            }
            if (currentScope && currentScope->hasVariableInCurrentScope(node->getVariableName())) {
                throw EnvironmentException("Variable '" + node->getVariableName() + "' shadows function parameter", node->getLocation());
            }
        }
        
        // Evaluate initial value if present
        Value initialValue = node->getInitializer() 
            ? mainEvaluator->evaluate(node->getInitializer())
            : std::monostate{};
        
        // Create variable definition
        auto varDef = std::make_shared<VariableDefinition>(
            node->getVariableName(),
            node->getType(),
            initialValue,
            node->isFinal()
        );
        
        // Register in environment
        env->declareVariable(node->getVariableName(), varDef);
        
        return initialValue;
    }

    Value StatementEvaluator::evaluateAssignmentNode(AssignmentNode* node)
    {
        auto env = mainEvaluator->getEnvironment();
        
        // Check if this is a variable declaration (has a non-VOID type) or an assignment
        if (node->getVariableType() != ValueType::VOID) {
            // This is a variable declaration
            // Check if variable already exists in current scope (but allow redefinitions during import evaluation)
            if (!env->isEvaluatingImport() && env->getScopeManager()->hasVariableInCurrentScope(node->getVariableName())) {
                throw EnvironmentException("Variable '" + node->getVariableName() + "' is already defined in this scope", node->getLocation());
            }
            
            // Also check for parameter shadowing - variables can't shadow function parameters
            if (!env->isEvaluatingImport() && env->isInFunction()) {
                auto currentScope = env->getScopeManager()->getCurrentScope();
                // Walk up to find the function scope and check for parameter conflicts
                while (currentScope && currentScope->getType() != ScopeType::FUNCTION) {
                    currentScope = currentScope->getParent();
                }
                if (currentScope && currentScope->hasVariableInCurrentScope(node->getVariableName())) {
                    throw EnvironmentException("Variable '" + node->getVariableName() + "' shadows function parameter", node->getLocation());
                }
            }
            
            Value initialValue;
            if (node->getValue()) {
                initialValue = mainEvaluator->evaluate(node->getValue());
                
                // Type checking: verify that the initial value matches the declared type
                ValueType actualType = getValueType(initialValue);
                ValueType declaredType = node->getVariableType();
                
                // Allow null assignment to any type, but check other type mismatches
                if (actualType != ValueType::NULL_TYPE && actualType != declaredType) {
                    throw EnvironmentException("Type mismatch: cannot assign " + valueTypeToString(actualType) + 
                                             " to variable of type " + valueTypeToString(declaredType), node->getLocation());
                }
                
                // For object types, also check class compatibility
                if (actualType == ValueType::OBJECT && declaredType == ValueType::OBJECT) {
                    const std::string& declaredClassName = node->getClassName();
                    if (!declaredClassName.empty()) {
                        // Get the actual class name of the assigned object
                        std::string actualClassName = "";
                        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(initialValue)) {
                            auto objInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(initialValue);
                            if (objInstance) {
                                actualClassName = objInstance->getTypeName();
                            }
                        }
                        
                        // Check if classes match (exact match for now - could be extended for inheritance)
                        if (!actualClassName.empty() && actualClassName != declaredClassName) {
                            throw TypeException("Type mismatch: cannot assign object of type '" + actualClassName + 
                                               "' to variable of type '" + declaredClassName + "'", node->getLocation());
                        }
                    }
                }
                
                // For object types, also validate that the class exists
                if (declaredType == ValueType::OBJECT) {
                    const std::string& className = node->getClassName();
                    if (!className.empty() && !env->findClass(className)) {
                        throw EnvironmentException("Undefined class: " + className, node->getLocation());
                    }
                }
            } else {
                // Default value based on type
                switch (node->getVariableType()) {
                    case ValueType::INT:
                        initialValue = Value(0);
                        break;
                    case ValueType::FLOAT:
                        initialValue = Value(0.0f);
                        break;
                    case ValueType::BOOL:
                        initialValue = Value(false);
                        break;
                    case ValueType::STRING:
                        initialValue = Value(std::string(""));
                        break;
                    case ValueType::OBJECT:
                        // For object types without initialization, validate that the class exists
                        {
                            const std::string& className = node->getClassName();
                            if (!className.empty() && !env->findClass(className)) {
                                throw EnvironmentException("Undefined class: " + className, node->getLocation());
                            }
                            initialValue = Value(nullptr);  // Default to null for objects
                        }
                        break;
                    default:
                        initialValue = Value();
                        break;
                }
            }
            
            // Create and register the variable
            auto varDef = std::make_shared<VariableDefinition>(node->getVariableName(), 
                                                             node->getVariableType(), 
                                                             initialValue, 
                                                             node->getIsFinal());
            env->declareVariable(node->getVariableName(), varDef);
            
            return initialValue;
        } else {
            // This is a regular assignment
            
            // First check if this might be a field assignment on the current instance
            // This should take precedence in constructor contexts to handle implicit field assignments
            auto currentInstance = mainEvaluator->getCurrentInstance();
            if (currentInstance) {
                auto field = currentInstance->getField(node->getVariableName());
                if (field) {
                    // This is a field assignment
                    if (field->isFinal()) {
                        throw TypeException("Cannot reassign final field: " + node->getVariableName(), node->getLocation());
                    }
                    
                    Value newValue = mainEvaluator->evaluate(node->getValue());
                    currentInstance->setField(node->getVariableName(), newValue);
                    return newValue;
                }
            }
            
            // If not a field assignment, then look for regular variable
            auto varDef = env->findVariable(node->getVariableName());
            
            if (!varDef) {
                
                // Check if this might be a static field assignment
                // When we're in a function scope, check if we can find a class with this static field
                if (env->isInFunction()) {
                    // Get function scope name to extract class name
                    std::string functionScope = env->getFunctionScopeName();
                    size_t pos = functionScope.find("::");
                    if (pos != std::string::npos) {
                        std::string className = functionScope.substr(0, pos);
                        auto classDef = env->findClass(className);
                        if (classDef) {
                            auto field = classDef->getField(node->getVariableName());
                            if (field && field->isStatic()) {
                                // This is a static field assignment
                                if (field->isFinal()) {
                                    throw TypeException("Cannot reassign final static field: " + node->getVariableName(), node->getLocation());
                                }
                                
                                Value newValue = mainEvaluator->evaluate(node->getValue());
                                field->setValue(newValue);
                                return newValue;
                            }
                        }
                    }
                }
                
                throw UndefinedException("Undefined variable: " + node->getVariableName(), node->getLocation());
            }
            
            if (varDef->isFinal()) {
                throw TypeException("Cannot reassign final variable: " + node->getVariableName(), node->getLocation());
            }
            
            Value newValue = mainEvaluator->evaluate(node->getValue());
            varDef->setValue(newValue);
            
            return newValue;
        }
    }

    Value StatementEvaluator::evaluateQualifiedAssignmentNode(QualifiedAssignmentNode* node)
    {
        // Delegate to NamespaceEvaluator through main evaluator
        return mainEvaluator->evaluateQualifiedAssignment(node);
    }

    Value StatementEvaluator::evaluateMemberAssignmentNode(MemberAssignmentNode* node)
    {
        // Delegate to ObjectEvaluator through main evaluator
        return mainEvaluator->evaluateObjectMemberAssignment(node);
    }

    Value StatementEvaluator::evaluateIfNode(IfNode* node)
    {
        Value condition = mainEvaluator->evaluate(node->getCondition());
        
        if (mainEvaluator->isTruthy(condition)) {
            return mainEvaluator->evaluate(node->getThenStatement());
        } else if (node->getElseStatement()) {
            return mainEvaluator->evaluate(node->getElseStatement());
        }
        
        return std::monostate{};
    }

    Value StatementEvaluator::evaluateWhileNode(WhileNode* node)
    {
        Value lastValue = std::monostate{};
        
        while (mainEvaluator->isTruthy(mainEvaluator->evaluate(node->getCondition()))) {
            try {
                lastValue = mainEvaluator->evaluate(node->getBody());
                
                if (isContinuing()) {
                    resetLoopFlags();
                    continue;
                }
                
                if (isBreaking()) {
                    resetLoopFlags();
                    break;
                }
                
                if (mainEvaluator->shouldReturn()) {
                    break;
                }
            }
            catch (const BreakException&) {
                break;
            }
            catch (const ContinueException&) {
                continue;
            }
        }
        
        // Reset break/continue flags when while loop completes
        resetLoopFlags();
        return lastValue;
    }

    Value StatementEvaluator::evaluateDoWhileNode(DoWhileNode* node)
    {
        Value lastValue = std::monostate{};
        
        do {
            try {
                lastValue = mainEvaluator->evaluate(node->getBody());
                
                if (isContinuing()) {
                    resetLoopFlags();
                    continue;
                }
                
                if (isBreaking()) {
                    resetLoopFlags();
                    break;
                }
                
                if (mainEvaluator->shouldReturn()) {
                    break;
                }
            }
            catch (const BreakException&) {
                break;
            }
            catch (const ContinueException&) {
                continue;
            }
        } while (mainEvaluator->isTruthy(mainEvaluator->evaluate(node->getCondition())));
        
        // Reset break/continue flags when do-while loop completes
        resetLoopFlags();
        return lastValue;
    }

    Value StatementEvaluator::evaluateForNode(ForNode* node)
    {
        auto env = mainEvaluator->getEnvironment();
        env->enterScope("", ScopeType::LOOP);
        
        Value lastValue = std::monostate{};
        
        try {
            // Initialize
            if (node->getInitialization()) {
                mainEvaluator->evaluate(node->getInitialization());
            }
            
            
            // Loop
            while (!node->getCondition() || mainEvaluator->isTruthy(mainEvaluator->evaluate(node->getCondition()))) {
                try {
                    lastValue = mainEvaluator->evaluate(node->getBody());
                    
                    if (mainEvaluator->shouldReturn()) {
                        break;
                    }
                    
                    // Update
                    if (node->getUpdate()) {
                        mainEvaluator->evaluate(node->getUpdate());
                    }
                }
                catch (const BreakException&) {
                    break;
                }
                catch (const ContinueException&) {
                    if (node->getUpdate()) {
                        mainEvaluator->evaluate(node->getUpdate());
                    }
                    continue;
                }
            }
        }
        catch (...) {
            env->exitScope();
            throw;
        }
        
        env->exitScope();
        // Reset break/continue flags when for loop completes
        resetLoopFlags();
        return lastValue;
    }

    Value StatementEvaluator::evaluateBreakNode(BreakNode* node)
    {
        breakFlag = true;
        throw BreakException();
    }

    Value StatementEvaluator::evaluateContinueNode(ContinueNode* node)
    {
        continueFlag = true;
        throw ContinueException();
    }

    Value StatementEvaluator::evaluateSwitchNode(SwitchNode* node)
    {
        Value switchValue = mainEvaluator->evaluate(node->getExpression());
        Value lastValue = std::monostate{};
        bool matched = false;
        
        // Check each case
        DefaultCaseNode* defaultCase = nullptr;
        
        for (const auto& casePtr : node->getCases()) {
            // Try to cast to CaseNode
            if (auto caseNode = dynamic_cast<CaseNode*>(casePtr.get())) {
                if (!matched) {
                    Value caseValue = mainEvaluator->evaluate(caseNode->getValue());
                    
                    // Proper value comparison using ExpressionEvaluator's comparison logic
                    matched = compareValues(switchValue, caseValue);
                }
                
                if (matched) {
                    try {
                        for (const auto& stmt : caseNode->getStatements()) {
                            lastValue = mainEvaluator->evaluate(stmt.get());
                            
                            if (isBreaking()) {
                                resetLoopFlags();
                                return lastValue;
                            }
                            
                            if (mainEvaluator->shouldReturn()) {
                                return lastValue;
                            }
                        }
                    }
                    catch (const exception::BreakException&) {
                        // Break from switch case - stop executing and return
                        resetLoopFlags();
                        return lastValue;
                    }
                    
                    // Fall through to next case
                }
            }
            // Try to cast to DefaultCaseNode
            else if (auto defaultCaseNode = dynamic_cast<DefaultCaseNode*>(casePtr.get())) {
                defaultCase = defaultCaseNode;
            }
        }
        
        // Execute default case if no match
        if (!matched && defaultCase) {
            try {
                for (const auto& stmt : defaultCase->getStatements()) {
                    lastValue = mainEvaluator->evaluate(stmt.get());
                    
                    if (isBreaking()) {
                        resetLoopFlags();
                        break;
                    }
                    
                    if (mainEvaluator->shouldReturn()) {
                        return lastValue;
                    }
                }
            }
            catch (const exception::BreakException&) {
                // Break from default case
                resetLoopFlags();
            }
        }
        
        return lastValue;
    }

    Value StatementEvaluator::evaluateCaseNode(CaseNode* node)
    {
        // Cases are evaluated as part of switch
        return std::monostate{};
    }

    Value StatementEvaluator::evaluateDefaultCaseNode(DefaultCaseNode* node)
    {
        // Default cases are evaluated as part of switch
        return std::monostate{};
    }

    Value StatementEvaluator::evaluateImportNode(ImportNode* node)
    {
        auto env = mainEvaluator->getEnvironment();
        const std::string& rawFilePath = node->getFilePath();
        
        // Get the ImportManager from the global environment/evaluator context
        // (since parser no longer sets it on ImportNode)
        ImportManager* importMgr = env->getImportManager();
        
        if (!importMgr) {
            throw EnvironmentException("ImportManager not available in evaluation context", node->getLocation());
        }
        
        // Resolve file path for consistent tracking
        std::string filePath = importMgr->resolvePath(rawFilePath);
        
        // Check if this file has already been evaluated
        if (importMgr->isEvaluated(filePath)) {
            // File already evaluated, completely skip all processing
            return std::monostate{};
        }
        
        // Set import evaluation flag for proper variable redefinition handling
        bool prevImportFlag = env->isEvaluatingImport();
        env->setImportEvaluation(true);
        
        try {
            // Check for circular dependency at evaluation level BEFORE adding to stack
            if (env->wouldCauseCircularImport(filePath)) {
                env->setImportEvaluation(prevImportFlag);
                throw EnvironmentException("Circular import detected: " + 
                                         env->getCircularImportChain(filePath), node->getLocation());
            }
            
            // Push to evaluation import stack to track circular dependencies across evaluation phases
            env->pushEvaluationImport(filePath);
            
            // Parse and cache the AST (pure parsing, no evaluation)
            ASTNode* importedAST = importMgr->parseAndCacheAST(rawFilePath);
            
            if (!importedAST) {
                env->setImportEvaluation(prevImportFlag);
                return std::monostate{};
            }
            
            // Now evaluate the imported AST in the current environment with import context
            if (auto programNode = dynamic_cast<ProgramNode*>(importedAST)) {
                for (const auto& statement : programNode->getStatements()) {
                    // Evaluate each declaration in the imported file
                    // This will register functions, variables, etc. in the current environment
                    mainEvaluator->evaluate(statement.get());
                }
            } else {
                // For any other AST node, just evaluate it directly
                mainEvaluator->evaluate(importedAST);
            }
            
            // Mark this file as evaluated AFTER successful evaluation
            importMgr->markAsEvaluated(filePath);
            
            // Pop from evaluation import stack and restore previous import flag
            env->popEvaluationImport();
            env->setImportEvaluation(prevImportFlag);
            return std::monostate{};
        }
        catch (const std::exception& e) {
            // Pop from evaluation import stack and restore previous import flag on exception
            env->popEvaluationImport();
            env->setImportEvaluation(prevImportFlag);
            throw EnvironmentException("Import evaluation error: " + std::string(e.what()), node->getLocation());
        }
        catch (...) {
            // Pop from evaluation import stack and restore previous import flag on exception
            env->popEvaluationImport();
            env->setImportEvaluation(prevImportFlag);
            throw;
        }
    }

    Value StatementEvaluator::evaluateFunctionNode(FunctionNode* node)
    {
        auto env = mainEvaluator->getEnvironment();
        
        // Create function definition with name, return type, and parameters
        auto funcDef = std::make_shared<FunctionDefinition>(
            node->getName(),
            node->getReturnType(),
            node->getParameters()
        );
        
        // Set the function body
        funcDef->setBody(node->getBody());
        
        // Register function
        env->registerFunction(node->getName(), funcDef);
        
        return std::monostate{};
    }

    Value StatementEvaluator::evaluateReturnNode(ReturnNode* node)
    {
        auto env = mainEvaluator->getEnvironment();
        
        Value returnValue = node->getReturnValue() 
            ? mainEvaluator->evaluate(node->getReturnValue())
            : std::monostate{};
            
        // Get current function name and check return type
        std::string currentFunctionName = env->getFunctionScopeName();
        if (!currentFunctionName.empty()) {
            auto functionDef = env->findFunction(currentFunctionName);
            if (functionDef) {
                ValueType expectedReturnType = functionDef->getReturnType();
                ValueType actualReturnType = getValueType(returnValue);
                
                // Check if return type matches (allow null returns for any type)
                if (actualReturnType != ValueType::NULL_TYPE && actualReturnType != expectedReturnType) {
                    throw EnvironmentException("Return type mismatch: cannot return " + 
                                             valueTypeToString(actualReturnType) + " from function expecting " + 
                                             valueTypeToString(expectedReturnType), node->getLocation());
                }
            }
        }
        
        mainEvaluator->pushReturnValue(returnValue);
        mainEvaluator->setReturned(true);
        throw ReturnException(returnValue);
    }

    Value StatementEvaluator::evaluateNativeFunctionNode(NativeFunctionNode* node)
    {
        // Native functions are registered by the environment
        return std::monostate{};
    }

    // Helper methods
    bool StatementEvaluator::shouldBreakOrContinue() const
    {
        return breakFlag || continueFlag;
    }

    void StatementEvaluator::handleBreak()
    {
        breakFlag = true;
    }

    void StatementEvaluator::handleContinue()
    {
        continueFlag = true;
    }

    bool StatementEvaluator::isBreaking() const
    {
        return breakFlag;
    }

    bool StatementEvaluator::isContinuing() const
    {
        return continueFlag;
    }

    void StatementEvaluator::resetLoopFlags()
    {
        breakFlag = false;
        continueFlag = false;
    }

    bool StatementEvaluator::compareValues(const Value& left, const Value& right)
    {
        // Handle null comparisons
        if (std::holds_alternative<nullptr_t>(left) && std::holds_alternative<nullptr_t>(right)) {
            return true;
        }
        if (std::holds_alternative<nullptr_t>(left) || std::holds_alternative<nullptr_t>(right)) {
            return false;
        }
        
        // Handle same type comparisons
        if (left.index() == right.index()) {
            if (std::holds_alternative<int>(left)) {
                return std::get<int>(left) == std::get<int>(right);
            }
            if (std::holds_alternative<float>(left)) {
                return std::get<float>(left) == std::get<float>(right);
            }
            if (std::holds_alternative<bool>(left)) {
                return std::get<bool>(left) == std::get<bool>(right);
            }
            if (std::holds_alternative<std::string>(left)) {
                return std::get<std::string>(left) == std::get<std::string>(right);
            }
            if (std::holds_alternative<std::monostate>(left)) {
                return true; // both are void
            }
        }
        
        // Handle numeric comparisons (int vs float)
        if ((std::holds_alternative<int>(left) && std::holds_alternative<float>(right)) ||
            (std::holds_alternative<float>(left) && std::holds_alternative<int>(right))) {
            float leftVal = mainEvaluator->toFloat(left);
            float rightVal = mainEvaluator->toFloat(right);
            return leftVal == rightVal;
        }
        
        return false; // Different types or unsupported comparison
    }
    
    ValueType StatementEvaluator::getValueType(const Value& value)
    {
        if (std::holds_alternative<int>(value)) {
            return ValueType::INT;
        }
        if (std::holds_alternative<float>(value)) {
            return ValueType::FLOAT;
        }
        if (std::holds_alternative<bool>(value)) {
            return ValueType::BOOL;
        }
        if (std::holds_alternative<std::string>(value)) {
            return ValueType::STRING;
        }
        if (std::holds_alternative<nullptr_t>(value)) {
            return ValueType::NULL_TYPE;
        }
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(value)) {
            return ValueType::OBJECT;
        }
        if (std::holds_alternative<std::monostate>(value)) {
            return ValueType::VOID;
        }
        
        return ValueType::VOID; // Default fallback
    }
    
    std::string StatementEvaluator::valueTypeToString(ValueType type)
    {
        switch (type) {
            case ValueType::INT:
                return "int";
            case ValueType::FLOAT:
                return "float";
            case ValueType::BOOL:
                return "bool";
            case ValueType::STRING:
                return "string";
            case ValueType::VOID:
                return "void";
            case ValueType::OBJECT:
                return "object";
            case ValueType::NULL_TYPE:
                return "null";
            default:
                return "unknown";
        }
    }
}