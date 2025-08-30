#include "ExpressionEvaluator.hpp"
#include "Evaluator.hpp"
#include "../errors/TypeException.hpp"
#include "../errors/MathException.hpp"
#include "../errors/UndefinedException.hpp"
#include "../errors/ArgumentException.hpp"
#include "../exception/ReturnException.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include <sstream>
#include <cmath>
#include <iostream>

namespace evaluator
{
    using namespace errors;
    using namespace runtimeTypes;
    using namespace runtimeTypes::global;
    using namespace runtimeTypes::klass;

    ExpressionEvaluator::ExpressionEvaluator(Evaluator* evaluator)
        : mainEvaluator(evaluator)
    {
    }

    Value ExpressionEvaluator::evaluateIntegerNode(IntegerNode* node)
    {
        return node->getValue();
    }

    Value ExpressionEvaluator::evaluateFloatNode(FloatNode* node)
    {
        return node->getValue();
    }

    Value ExpressionEvaluator::evaluateStringNode(StringNode* node)
    {
        return node->getValue();
    }

    Value ExpressionEvaluator::evaluateBoolNode(BoolNode* node)
    {
        return node->getValue();
    }

    Value ExpressionEvaluator::evaluateNullNode(NullNode* node)
    {
        return nullptr;
    }

    Value ExpressionEvaluator::evaluateVariableNode(VariableNode* node)
    {
        // Handle 'this' keyword specifically
        if (node->getName() == "this") {
            auto currentInstance = mainEvaluator->getCurrentInstance();
            if (currentInstance) {
                return currentInstance;
            }
            throw UndefinedException("'this' is not available outside of instance methods", node->getLocation());
        }
        
        auto env = mainEvaluator->getEnvironment();
        
        
        auto varDef = env->findVariable(node->getName());
        
        if (!varDef) {
            // Check if this might be a field access on the current instance
            auto currentInstance = mainEvaluator->getCurrentInstance();
            if (currentInstance) {
                auto field = currentInstance->getField(node->getName());
                if (field) {
                    return currentInstance->getFieldValue(node->getName());
                }
            }
            
            // Check if this might be a static field access
            // First check if we're in a static method by looking for the current class name
            auto env = mainEvaluator->getEnvironment();
            auto classRegistry = env->getClassRegistry();
            
            // Check if we have a current class name stored (from static method execution)
            auto currentClassVar = env->findVariable("__current_class_name__");
            if (currentClassVar) {
                auto currentClassValue = currentClassVar->getValue();
                if (std::holds_alternative<std::string>(currentClassValue)) {
                    std::string className = std::get<std::string>(currentClassValue);
                    auto classDef = env->findClass(className);
                    if (classDef) {
                        auto field = classDef->getField(node->getName());
                        if (field && field->isStatic()) {
                            return field->getValue();
                        }
                    }
                }
            }
            
            // Fallback: search all classes (for backward compatibility)
            auto allClassNames = classRegistry->getAllItemNames();
            std::string currentClassName;
            if (currentClassVar) {
                auto currentClassValue = currentClassVar->getValue();
                if (std::holds_alternative<std::string>(currentClassValue)) {
                    currentClassName = std::get<std::string>(currentClassValue);
                }
            }
            
            for (const auto& className : allClassNames) {
                // Skip if we already checked this class above
                if (!currentClassName.empty() && className == currentClassName) continue;
                
                auto classDef = classRegistry->findClass(className);
                if (classDef) {
                    auto field = classDef->getField(node->getName());
                    if (field && field->isStatic()) {
                        return field->getValue();
                    }
                }
            }
            
            throw UndefinedException("Undefined variable: " + node->getName(), node->getLocation());
        }
        
        return varDef->getValue();
    }

    Value ExpressionEvaluator::evaluateBinaryExpNode(BinaryExpNode* node)
    {
        Value left = mainEvaluator->evaluate(node->getLeft());
        TokenType op = node->getOperator();
        
        // Handle short-circuit evaluation for logical operators
        if (op == TokenType::AND) {
            if (!isTruthy(left)) {
                return false;
            }
            Value right = mainEvaluator->evaluate(node->getRight());
            return isTruthy(right);
        }
        else if (op == TokenType::OR) {
            if (isTruthy(left)) {
                return true;
            }
            Value right = mainEvaluator->evaluate(node->getRight());
            return isTruthy(right);
        }
        
        Value right = mainEvaluator->evaluate(node->getRight());
        
        // Handle different operator categories
        switch (op) {
            case TokenType::PLUS:
            case TokenType::MINUS:
            case TokenType::MULTIPLY:
            case TokenType::DIVIDE:
            case TokenType::MODULO:
                return evaluateArithmetic(left, right, op);
                
            case TokenType::EQUALS:
            case TokenType::NOT_EQUALS:
            case TokenType::LESS:
            case TokenType::LESS_EQUALS:
            case TokenType::GREATER:
            case TokenType::GREATER_EQUALS:
                return evaluateComparison(left, right, op);
                
            default:
                throw TypeException("Unknown binary operator", node->getLocation());
        }
    }

    Value ExpressionEvaluator::evaluateTernaryExpNode(TernaryExpNode* node)
    {
        Value condition = mainEvaluator->evaluate(node->getCondition());
        
        if (isTruthy(condition)) {
            return mainEvaluator->evaluate(node->getTrueExpression());
        } else {
            return mainEvaluator->evaluate(node->getFalseExpression());
        }
    }

    Value ExpressionEvaluator::evaluateUnaryExpNode(UnaryExpNode* node)
    {
        TokenType op = node->getOperator();
        
        // Handle increment and decrement operators (require variable operand)
        if (op == TokenType::INCREMENT || op == TokenType::DECREMENT) {
            // For increment/decrement, we need to modify the variable, so we need a VariableNode
            auto varNode = dynamic_cast<VariableNode*>(node->getOperand());
            if (!varNode) {
                throw TypeException("Increment/decrement operators can only be applied to variables", node->getLocation());
            }
            
            auto env = mainEvaluator->getEnvironment();
            auto varDef = env->findVariable(varNode->getName());
            if (!varDef) {
                throw UndefinedException("Undefined variable: " + varNode->getName(), node->getLocation());
            }
            
            Value currentValue = varDef->getValue();
            
            // Check if it's a numeric type
            if (!std::holds_alternative<int>(currentValue) && !std::holds_alternative<float>(currentValue)) {
                throw TypeException("Increment/decrement operators can only be applied to numeric values", node->getLocation());
            }
            
            Value newValue;
            Value returnValue;
            
            // Calculate new value based on operator
            if (std::holds_alternative<int>(currentValue)) {
                int intVal = std::get<int>(currentValue);
                int newIntVal = (op == TokenType::INCREMENT) ? intVal + 1 : intVal - 1;
                newValue = newIntVal;
                
                // For postfix, return original value; for prefix, return new value
                returnValue = node->isPostfix() ? intVal : newIntVal;
            } else {
                float floatVal = std::get<float>(currentValue);
                float newFloatVal = (op == TokenType::INCREMENT) ? floatVal + 1.0f : floatVal - 1.0f;
                newValue = newFloatVal;
                
                // For postfix, return original value; for prefix, return new value
                returnValue = node->isPostfix() ? floatVal : newFloatVal;
            }
            
            // Update the variable
            varDef->setValue(newValue);
            return returnValue;
        }
        
        // Handle other unary operators (evaluate operand first)
        Value operand = mainEvaluator->evaluate(node->getOperand());
        
        switch (op) {
            case TokenType::MINUS:
                if (std::holds_alternative<int>(operand)) {
                    return -std::get<int>(operand);
                }
                if (std::holds_alternative<float>(operand)) {
                    return -std::get<float>(operand);
                }
                throw TypeException("Cannot apply unary minus to non-numeric value", SourceLocation{});
                
            case TokenType::PLUS:
                if (std::holds_alternative<int>(operand) || std::holds_alternative<float>(operand)) {
                    return operand; // Unary plus returns the value as-is
                }
                throw TypeException("Cannot apply unary plus to non-numeric value", SourceLocation{});
                
            case TokenType::NOT:
                return !isTruthy(operand);
                
            default:
                throw TypeException("Unknown unary operator", SourceLocation{});
        }
    }

    Value ExpressionEvaluator::evaluateFunctionCallNode(FunctionCallNode* node)
    {
        auto env = mainEvaluator->getEnvironment();
        
        // First check if it's a native function
        auto nativeRegistry = env->getNativeRegistry();
        if (nativeRegistry->hasNativeFunction(node->getFunctionName())) {
            auto nativeFunc = nativeRegistry->findNativeFunction(node->getFunctionName());
            
            // Evaluate arguments
            std::vector<Value> args;
            for (auto& argNode : node->getArguments()) {
                args.push_back(mainEvaluator->evaluate(argNode.get()));
            }
            
            // Call native function
            return nativeFunc(args);
        }
        
        // Check if this is a qualified call (contains ::)
        std::string functionName = node->getFunctionName();
        if (functionName.find("::") != std::string::npos) {
            // Parse the qualified name into parts
            std::vector<std::string> parts;
            size_t start = 0;
            size_t pos = 0;
            while ((pos = functionName.find("::", start)) != std::string::npos) {
                parts.push_back(functionName.substr(start, pos - start));
                start = pos + 2;
            }
            parts.push_back(functionName.substr(start));
            
            // First, try to find it as a namespace function
            if (parts.size() >= 2) {
                std::vector<std::string> namespacePath(parts.begin(), parts.end() - 1);
                std::string funcName = parts.back();
                
                auto functionRegistry = env->getFunctionRegistry();
                auto functionDef = functionRegistry->findFunctionInNamespace(namespacePath, funcName);
                
                // If not found with absolute path, try relative to current namespace
                if (!functionDef) {
                    auto currentNamespacePath = env->getCurrentNamespacePath();
                    if (!currentNamespacePath.empty()) {
                        std::vector<std::string> relativeNamespacePath = currentNamespacePath;
                        relativeNamespacePath.insert(relativeNamespacePath.end(), namespacePath.begin(), namespacePath.end());
                        functionDef = functionRegistry->findFunctionInNamespace(relativeNamespacePath, funcName);
                    }
                }
                
                if (functionDef) {
                    // Found a namespace function - evaluate it
                    std::vector<Value> args;
                    for (auto& argNode : node->getArguments()) {
                        args.push_back(mainEvaluator->evaluate(argNode.get()));
                    }
                    
                    // Check parameter count
                    if (args.size() != functionDef->getParameterCount()) {
                        throw ArgumentException("Function '" + functionName + "' expects " + 
                                              std::to_string(functionDef->getParameterCount()) +
                                              " arguments, got " + std::to_string(args.size()),
                                              node->getLocation());
                    }
                    
                    // Save the current return state
                    bool savedReturnState = mainEvaluator->shouldReturn();
                    
                    // Save current namespace context and set function's namespace context
                    auto savedNamespacePath = env->getCurrentNamespacePath();
                    const auto& functionNamespace = functionDef->getNamespaceContext();
                    if (!functionNamespace.empty()) {
                        env->enterNamespace(functionNamespace);
                        
                        // Apply namespace's using directives
                        auto nsManager = env->getNamespaceManager();
                        if (nsManager) {
                            auto currentNs = nsManager->getCurrentNamespace();
                            if (currentNs) {
                                const auto& nsUsingDirs = currentNs->getUsingDirectives();
                                for (const auto& usingDir : nsUsingDirs) {
                                    nsManager->addUsingDirective(usingDir);
                                }
                            }
                        }
                    }
                    
                    // Create new scope for function execution
                    env->enterScope(functionName, ScopeType::FUNCTION);
                    
                    // Bind parameters to arguments
                    const auto& parameters = functionDef->getParameters();
                    for (size_t i = 0; i < args.size(); ++i) {
                        auto varDef = std::make_shared<VariableDefinition>(
                            parameters[i].first,   // parameter name
                            parameters[i].second,  // parameter type
                            args[i]
                        );
                        env->declareVariable(parameters[i].first, varDef);
                    }
                    
                    // Execute function body
                    Value result = std::monostate{};
                    try {
                        result = mainEvaluator->evaluate(functionDef->getBody());
                        
                        // Extract return value if the function returned
                        if (mainEvaluator->shouldReturn()) {
                            result = mainEvaluator->getReturnValue();
                        }
                    }
                    catch (const exception::ReturnException& e) {
                        // Handle return statement - this is the expected way functions return
                        env->exitScope();
                        // Restore namespace context
                        if (!functionNamespace.empty()) {
                            env->exitNamespace();
                            if (!savedNamespacePath.empty()) {
                                env->enterNamespace(savedNamespacePath);
                            } else {
                                // Ensure we're in global namespace - fix for scope stack imbalance
                                while (!env->getCurrentNamespacePath().empty()) {
                                    env->exitNamespace();
                                }
                            }
                        }
                        // Reset return state since this was a function return, not a program return
                        mainEvaluator->setReturned(savedReturnState);
                        return e.returnValue;
                    }
                    catch (...) {
                        env->exitScope();
                        // Restore namespace context
                        if (!functionNamespace.empty()) {
                            env->exitNamespace();
                            if (!savedNamespacePath.empty()) {
                                env->enterNamespace(savedNamespacePath);
                            } else {
                                // Ensure we're in global namespace - fix for scope stack imbalance
                                while (!env->getCurrentNamespacePath().empty()) {
                                    env->exitNamespace();
                                }
                            }
                        }
                        mainEvaluator->setReturned(savedReturnState);
                        throw;
                    }
                    
                    env->exitScope();
                    // Restore namespace context
                    if (!functionNamespace.empty()) {
                        env->exitNamespace();
                        if (!savedNamespacePath.empty()) {
                            env->enterNamespace(savedNamespacePath);
                        } else {
                            // Ensure we're in global namespace - fix for scope stack imbalance
                            while (!env->getCurrentNamespacePath().empty()) {
                                env->exitNamespace();
                            }
                        }
                    }
                    mainEvaluator->setReturned(savedReturnState);
                    
                    return result;
                }
            }
            
            // If not found as namespace function, try as static method
            if (parts.size() >= 2) {
                // For qualified static method calls: namespace::ClassName::methodName
                // The last part is method name, everything else is the qualified class name
                std::string methodName = parts.back();
                std::vector<std::string> classNameParts(parts.begin(), parts.end() - 1);
                
                // Join the class name parts with "::" to form qualified class name
                std::string className = "";
                for (size_t i = 0; i < classNameParts.size(); ++i) {
                    if (i > 0) className += "::";
                    className += classNameParts[i];
                }
                
                // Find the class definition
                auto classDef = env->findClass(className);
                if (!classDef) {
                    throw UndefinedException("Undefined class or namespace: " + className, node->getLocation());
                }
            
            // Find the static method within the class
            auto method = classDef->getMethod(methodName);
            if (!method) {
                throw UndefinedException("Method '" + methodName + "' not found in class '" + className + "'", node->getLocation());
            }
            
            if (!method->isStatic()) {
                throw TypeException("Cannot call non-static method '" + methodName + "' on class '" + className + "'", node->getLocation());
            }
            
            // Evaluate arguments
            std::vector<Value> args;
            for (auto& argNode : node->getArguments()) {
                args.push_back(mainEvaluator->evaluate(argNode.get()));
            }
            
            // Check parameter count
            if (args.size() != method->getParameters().size()) {
                throw ArgumentException("Method '" + methodName + "' expects " + 
                                      std::to_string(method->getParameters().size()) +
                                      " arguments, got " + std::to_string(args.size()),
                                      node->getLocation());
            }
            
            // Save the current return state to isolate method execution
            bool savedReturnState = mainEvaluator->shouldReturn();
            
            // Save current namespace context and enter the class's namespace
            auto savedNamespacePath = env->getCurrentNamespacePath();
            auto classNamespace = classDef->getNamespaceContext();
            if (!classNamespace.empty()) {
                env->enterNamespace(classNamespace);
            }
            
            // Create new scope for method execution
            std::string scopeName = className + "::" + methodName;
            env->enterScope(scopeName, ScopeType::FUNCTION);
            
            // Store the current class name in a scope variable for static field access
            // Since we've entered the namespace context, we need to store the unqualified class name
            std::string unqualifiedClassName = className;
            if (className.find("::") != std::string::npos) {
                // Extract the last part (the actual class name without namespace)
                unqualifiedClassName = className.substr(className.find_last_of("::") + 1);
            }
            auto currentClassVar = std::make_shared<VariableDefinition>(
                "__current_class_name__", ValueType::STRING, Value(unqualifiedClassName), true);
            env->declareVariable("__current_class_name__", currentClassVar);
            
            // Bind parameters to arguments
            for (size_t i = 0; i < args.size(); ++i) {
                const auto& param = method->getParameters()[i];
                auto varDef = std::make_shared<VariableDefinition>(
                    param.first,   // parameter name
                    param.second,  // parameter type
                    args[i],       // argument value
                    false          // not final
                );
                env->declareVariable(param.first, varDef);
            }
            
            // Reset return state for method execution
            mainEvaluator->setReturned(false);
            
            // Execute the static method body
            Value result = std::monostate{};
            try {
                mainEvaluator->evaluate(method->getBody());
                
                if (mainEvaluator->shouldReturn()) {
                    result = mainEvaluator->getReturnValue();
                }
            }
            catch (const exception::ReturnException& e) {
                // This is expected - return statement was executed
                result = e.returnValue;
            }
            catch (...) {
                env->exitScope();
                
                // Restore namespace context
                if (!classNamespace.empty()) {
                    env->exitNamespace();
                    if (!savedNamespacePath.empty()) {
                        env->enterNamespace(savedNamespacePath);
                    } else {
                        // Ensure we're in global namespace - fix for scope stack imbalance
                        while (!env->getCurrentNamespacePath().empty()) {
                            env->exitNamespace();
                        }
                    }
                }
                
                // Restore return state before throwing
                mainEvaluator->setReturned(savedReturnState);
                throw;
            }
            
            env->exitScope();
            
            // Restore namespace context
            if (!classNamespace.empty()) {
                env->exitNamespace();
                if (!savedNamespacePath.empty()) {
                    env->enterNamespace(savedNamespacePath);
                }
            }
            
            // Restore the original return state to not affect outer context
            mainEvaluator->setReturned(savedReturnState);
            
            return result;
            }
        }
        
        // First check if we're in a method context and this could be a method call
        auto currentInstance = mainEvaluator->getCurrentInstance();
        if (currentInstance) {
            auto method = currentInstance->getClassDefinition()->getMethod(node->getFunctionName());
            if (method && !method->isStatic()) {
                // This is a method call on the current instance (recursive or other method call)
                std::vector<Value> args;
                for (auto& argNode : node->getArguments()) {
                    args.push_back(mainEvaluator->evaluate(argNode.get()));
                }
                
                return mainEvaluator->callMethodOnInstance(currentInstance, node->getFunctionName(), args);
            }
        }
        
        // Check for user-defined function
        auto funcDef = env->findFunction(node->getFunctionName());
        
        if (!funcDef) {
            throw UndefinedException("Undefined function: " + node->getFunctionName(), node->getLocation());
        }
        
        // Evaluate arguments
        std::vector<Value> args;
        for (auto& argNode : node->getArguments()) {
            args.push_back(mainEvaluator->evaluate(argNode.get()));
        }
        
        // Check parameter count
        if (args.size() != funcDef->getParameters().size()) {
            throw ArgumentException("Function '" + node->getFunctionName() + 
                                  "' expects " + std::to_string(funcDef->getParameters().size()) +
                                  " arguments, got " + std::to_string(args.size()),
                                  node->getLocation());
        }
        
        // Save current namespace context and set function's namespace context
        auto savedNamespacePath = env->getCurrentNamespacePath();
        const auto& functionNamespace = funcDef->getNamespaceContext();
        if (!functionNamespace.empty()) {
            env->enterNamespace(functionNamespace);
            
            // Apply the namespace's using directives for function execution
            auto nsManager = env->getNamespaceManager();
            if (nsManager) {
                auto namespaceDef = nsManager->findNamespace(functionNamespace);
                if (namespaceDef) {
                    const auto& usingDirectives = namespaceDef->getUsingDirectives();
                    for (const auto& usingDir : usingDirectives) {
                        nsManager->addUsingDirective(usingDir);
                    }
                }
            }
        }
        
        // Create new scope for function execution
        env->enterScope(node->getFunctionName(), ScopeType::FUNCTION);
        
        // Bind parameters to arguments
        for (size_t i = 0; i < args.size(); ++i) {
            const auto& param = funcDef->getParameters()[i];
            auto varDef = std::make_shared<VariableDefinition>(
                param.first,   // parameter name
                param.second,  // parameter type
                args[i],       // argument value
                false          // not final
            );
            env->declareVariable(param.first, varDef);
        }
        
        // Execute function body
        Value result = std::monostate{};
        try {
            mainEvaluator->evaluate(funcDef->getBody());
            
            // Get return value if function returned
            if (mainEvaluator->shouldReturn()) {
                result = mainEvaluator->getReturnValue();
                mainEvaluator->setReturned(false);
            }
        }
        catch (const exception::ReturnException& e) {
            // Handle return statement - this is the expected way functions return
            env->exitScope();
            // Restore namespace context
            if (!functionNamespace.empty()) {
                env->exitNamespace();
                if (!savedNamespacePath.empty()) {
                    env->enterNamespace(savedNamespacePath);
                } else {
                    // Ensure we're in global namespace - fix for scope stack imbalance
                    while (!env->getCurrentNamespacePath().empty()) {
                        env->exitNamespace();
                    }
                }
            }
            // Reset return state since this was a function return, not a program return
            mainEvaluator->setReturned(false);
            return e.returnValue;
        }
        catch (...) {
            env->exitScope();
            // Restore namespace context
            if (!functionNamespace.empty()) {
                env->exitNamespace();
                if (!savedNamespacePath.empty()) {
                    env->enterNamespace(savedNamespacePath);
                } else {
                    // Ensure we're in global namespace - fix for scope stack imbalance
                    while (!env->getCurrentNamespacePath().empty()) {
                        env->exitNamespace();
                    }
                }
            }
            throw;
        }
        
        env->exitScope();
        // Restore namespace context
        if (!functionNamespace.empty()) {
            env->exitNamespace();
            if (!savedNamespacePath.empty()) {
                env->enterNamespace(savedNamespacePath);
            } else {
                // Ensure we're in global namespace - fix for scope stack imbalance
                while (!env->getCurrentNamespacePath().empty()) {
                    env->exitNamespace();
                }
            }
        }
        return result;
    }

    Value ExpressionEvaluator::evaluateMemberAccessNode(MemberAccessNode* node)
    {
        Value objectValue = mainEvaluator->evaluate(node->getObject());
        
        // Check if object is null
        if (std::holds_alternative<nullptr_t>(objectValue)) {
            throw TypeException("Cannot access member of null object", node->getLocation());
        }
        
        // Get object instance
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue)) {
            throw TypeException("Cannot access member of non-object value", node->getLocation());
        }
        
        auto object = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);
        auto field = object->getField(node->getMemberName());
        
        if (!field) {
            throw UndefinedException("Undefined field: " + node->getMemberName(), node->getLocation());
        }
        
        return field->getValue();
    }

    Value ExpressionEvaluator::evaluateMethodCallNode(MethodCallNode* node)
    {
        Value objectValue = mainEvaluator->evaluate(node->getObject());
        
        // Check if object is null
        if (std::holds_alternative<nullptr_t>(objectValue)) {
            throw TypeException("Cannot call method on null object", node->getLocation());
        }
        
        // Get object instance
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue)) {
            throw TypeException("Cannot call method on non-object value", node->getLocation());
        }
        
        auto object = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);
        
        // Evaluate arguments
        std::vector<Value> args;
        for (auto& argNode : node->getArguments()) {
            args.push_back(mainEvaluator->evaluate(argNode.get()));
        }
        
        // Delegate to ObjectEvaluator through main evaluator
        return mainEvaluator->evaluateObjectMethodCall(node);
    }

    Value ExpressionEvaluator::evaluateNewNode(NewNode* node)
    {
        // Delegate to ObjectEvaluator through main evaluator
        return mainEvaluator->evaluateObjectCreation(node);
    }

    Value ExpressionEvaluator::evaluateQualifiedNameNode(QualifiedNameNode* node)
    {
        // Delegate to NamespaceEvaluator through main evaluator
        return mainEvaluator->evaluateQualifiedNameAccess(node);
    }

    // Helper methods for binary operations
    Value ExpressionEvaluator::evaluateArithmetic(const Value& left, const Value& right, TokenType op)
    {
        // Handle string concatenation for PLUS operator
        if (op == TokenType::PLUS && 
            (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right))) {
            return toString(left) + toString(right);
        }
        
        // Handle numeric operations
        bool isFloat = std::holds_alternative<float>(left) || std::holds_alternative<float>(right);
        
        if (isFloat) {
            float leftVal = toFloat(left);
            float rightVal = toFloat(right);
            
            switch (op) {
                case TokenType::PLUS: return leftVal + rightVal;
                case TokenType::MINUS: return leftVal - rightVal;
                case TokenType::MULTIPLY: return leftVal * rightVal;
                case TokenType::DIVIDE:
                    if (rightVal == 0.0f) {
                        throw MathException("Division by zero", SourceLocation{});
                    }
                    return leftVal / rightVal;
                case TokenType::MODULO:
                    if (rightVal == 0.0f) {
                        throw MathException("Modulo by zero", SourceLocation{});
                    }
                    return std::fmod(leftVal, rightVal);
                default:
                    throw TypeException("Invalid arithmetic operator", SourceLocation{});
            }
        } else {
            int leftVal = toInt(left);
            int rightVal = toInt(right);
            
            switch (op) {
                case TokenType::PLUS: return leftVal + rightVal;
                case TokenType::MINUS: return leftVal - rightVal;
                case TokenType::MULTIPLY: return leftVal * rightVal;
                case TokenType::DIVIDE:
                    if (rightVal == 0) {
                        throw MathException("Division by zero", SourceLocation{});
                    }
                    return leftVal / rightVal;
                case TokenType::MODULO:
                    if (rightVal == 0) {
                        throw MathException("Modulo by zero", SourceLocation{});
                    }
                    return leftVal % rightVal;
                default:
                    throw TypeException("Invalid arithmetic operator", SourceLocation{});
            }
        }
    }

    Value ExpressionEvaluator::evaluateComparison(const Value& left, const Value& right, TokenType op)
    {
        // Handle null comparisons
        if (std::holds_alternative<nullptr_t>(left) || std::holds_alternative<nullptr_t>(right)) {
            bool leftNull = std::holds_alternative<nullptr_t>(left);
            bool rightNull = std::holds_alternative<nullptr_t>(right);
            
            switch (op) {
                case TokenType::EQUALS: return leftNull == rightNull;
                case TokenType::NOT_EQUALS: return leftNull != rightNull;
                default:
                    throw TypeException("Cannot compare null with relational operators", SourceLocation{});
            }
        }
        
        // Handle string comparisons
        if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right)) {
            std::string leftStr = std::get<std::string>(left);
            std::string rightStr = std::get<std::string>(right);
            
            switch (op) {
                case TokenType::EQUALS: return leftStr == rightStr;
                case TokenType::NOT_EQUALS: return leftStr != rightStr;
                case TokenType::LESS: return leftStr < rightStr;
                case TokenType::LESS_EQUALS: return leftStr <= rightStr;
                case TokenType::GREATER: return leftStr > rightStr;
                case TokenType::GREATER_EQUALS: return leftStr >= rightStr;
                default:
                    throw TypeException("Invalid comparison operator", SourceLocation{});
            }
        }
        
        // Handle boolean comparisons
        if (std::holds_alternative<bool>(left) && std::holds_alternative<bool>(right)) {
            bool leftBool = std::get<bool>(left);
            bool rightBool = std::get<bool>(right);
            
            switch (op) {
                case TokenType::EQUALS: return leftBool == rightBool;
                case TokenType::NOT_EQUALS: return leftBool != rightBool;
                default:
                    throw TypeException("Cannot compare booleans with relational operators", SourceLocation{});
            }
        }
        
        // Handle numeric comparisons
        bool isFloat = std::holds_alternative<float>(left) || std::holds_alternative<float>(right);
        
        if (isFloat) {
            float leftVal = toFloat(left);
            float rightVal = toFloat(right);
            
            switch (op) {
                case TokenType::EQUALS: return leftVal == rightVal;
                case TokenType::NOT_EQUALS: return leftVal != rightVal;
                case TokenType::LESS: return leftVal < rightVal;
                case TokenType::LESS_EQUALS: return leftVal <= rightVal;
                case TokenType::GREATER: return leftVal > rightVal;
                case TokenType::GREATER_EQUALS: return leftVal >= rightVal;
                default:
                    throw TypeException("Invalid comparison operator", SourceLocation{});
            }
        } else {
            int leftVal = toInt(left);
            int rightVal = toInt(right);
            
            switch (op) {
                case TokenType::EQUALS: return leftVal == rightVal;
                case TokenType::NOT_EQUALS: return leftVal != rightVal;
                case TokenType::LESS: return leftVal < rightVal;
                case TokenType::LESS_EQUALS: return leftVal <= rightVal;
                case TokenType::GREATER: return leftVal > rightVal;
                case TokenType::GREATER_EQUALS: return leftVal >= rightVal;
                default:
                    throw TypeException("Invalid comparison operator", SourceLocation{});
            }
        }
    }

    Value ExpressionEvaluator::evaluateLogical(const Value& left, const Value& right, TokenType op)
    {
        bool leftBool = isTruthy(left);
        bool rightBool = isTruthy(right);
        
        switch (op) {
            case TokenType::AND: return leftBool && rightBool;
            case TokenType::OR: return leftBool || rightBool;
            default:
                throw TypeException("Invalid logical operator", SourceLocation{});
        }
    }

    Value ExpressionEvaluator::evaluateStringOperation(const Value& left, const Value& right, TokenType op)
    {
        if (op == TokenType::PLUS) {
            return toString(left) + toString(right);
        }
        throw TypeException("Invalid string operator", SourceLocation{});
    }

    // Type coercion helpers
    bool ExpressionEvaluator::isTruthy(const Value& value)
    {
        if (std::holds_alternative<bool>(value)) {
            return std::get<bool>(value);
        }
        if (std::holds_alternative<nullptr_t>(value)) {
            return false;
        }
        if (std::holds_alternative<int>(value)) {
            return std::get<int>(value) != 0;
        }
        if (std::holds_alternative<float>(value)) {
            return std::get<float>(value) != 0.0f;
        }
        if (std::holds_alternative<std::string>(value)) {
            return !std::get<std::string>(value).empty();
        }
        if (std::holds_alternative<std::monostate>(value)) {
            return false;
        }
        // Object instances are truthy if not null
        return true;
    }

    float ExpressionEvaluator::toFloat(const Value& value)
    {
        if (std::holds_alternative<float>(value)) {
            return std::get<float>(value);
        }
        if (std::holds_alternative<int>(value)) {
            return static_cast<float>(std::get<int>(value));
        }
        if (std::holds_alternative<bool>(value)) {
            return std::get<bool>(value) ? 1.0f : 0.0f;
        }
        throw TypeException("Cannot convert to float", SourceLocation{});
    }

    int ExpressionEvaluator::toInt(const Value& value)
    {
        if (std::holds_alternative<int>(value)) {
            return std::get<int>(value);
        }
        if (std::holds_alternative<float>(value)) {
            return static_cast<int>(std::get<float>(value));
        }
        if (std::holds_alternative<bool>(value)) {
            return std::get<bool>(value) ? 1 : 0;
        }
        throw TypeException("Cannot convert to int", SourceLocation{});
    }

    std::string ExpressionEvaluator::toString(const Value& value)
    {
        if (std::holds_alternative<std::string>(value)) {
            return std::get<std::string>(value);
        }
        if (std::holds_alternative<int>(value)) {
            return std::to_string(std::get<int>(value));
        }
        if (std::holds_alternative<float>(value)) {
            return std::to_string(std::get<float>(value));
        }
        if (std::holds_alternative<bool>(value)) {
            return std::get<bool>(value) ? "true" : "false";
        }
        if (std::holds_alternative<nullptr_t>(value)) {
            return "null";
        }
        if (std::holds_alternative<std::monostate>(value)) {
            return "";
        }
        // For objects, return a string representation
        return "[object]";
    }
}