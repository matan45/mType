#include "StatementEvaluator.hpp"
#include "Evaluator.hpp"
#include "ExpressionEvaluator.hpp"
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
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include <iostream>

namespace evaluator
{
    using namespace errors;
    using namespace exception;
    using namespace runtimeTypes::global;
    using namespace environment::manager;

    StatementEvaluator::StatementEvaluator(Evaluator* evaluator)
        : mainEvaluator(evaluator), breakFlag(false), continueFlag(false)
    {
    }

    Value StatementEvaluator::evaluateProgramNode(ProgramNode* node)
    {
        Value lastValue = std::monostate{};
        
        for (const auto& statement : node->getStatements()) {
            lastValue = mainEvaluator->evaluate(statement.get());
            
            if (mainEvaluator->shouldReturn()) {
                break;
            }
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
                
                if (mainEvaluator->shouldReturn() || shouldBreakOrContinue()) {
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
            Value initialValue;
            if (node->getValue()) {
                initialValue = mainEvaluator->evaluate(node->getValue());
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
            auto varDef = env->findVariable(node->getVariableName());
            
            if (!varDef) {
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
                    
                    if (isContinuing()) {
                        resetLoopFlags();
                    } else if (isBreaking()) {
                        resetLoopFlags();
                        break;
                    } else if (mainEvaluator->shouldReturn()) {
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
        ASTNode* importedAST = node->getImportedAST();
        
        if (!importedAST) {
            return std::monostate{};
        }
        
        // If the imported AST is a ProgramNode, evaluate all its statements
        // to register functions and variables in the current environment
        if (auto programNode = dynamic_cast<ProgramNode*>(importedAST)) {
            for (const auto& statement : programNode->getStatements()) {
                // Evaluate each declaration in the imported file
                // This will register functions, variables, etc. in the current environment
                mainEvaluator->evaluate(statement.get());
            }
        } else {
            // If it's a single statement, just evaluate it
            mainEvaluator->evaluate(importedAST);
        }
        
        return std::monostate{};
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
        Value returnValue = node->getReturnValue() 
            ? mainEvaluator->evaluate(node->getReturnValue())
            : std::monostate{};
        
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
}