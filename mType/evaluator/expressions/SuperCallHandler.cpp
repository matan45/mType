#include "SuperCallHandler.hpp"
#include "../../ast/nodes/classes/SuperMethodCallNode.hpp"
#include "../utils/ParameterBinder.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"
#include "../../errors/UndefinedException.hpp"
#include "../../errors/ReturnException.hpp"
#include "../../value/PromiseValue.hpp"
#include <unordered_set>

using namespace errors;
using namespace runtimeTypes::klass;
using namespace runtimeTypes::global;

namespace evaluator
{
    namespace expressions
    {
        SuperCallHandler::SuperCallHandler(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr), stmtEvaluator(nullptr), objEvaluator(nullptr)
        {
        }

        void SuperCallHandler::setExpressionEvaluator(interfaces::IExpressionEvaluator* evaluator)
        {
            exprEvaluator = evaluator;
        }

        void SuperCallHandler::setStatementEvaluator(interfaces::IStatementEvaluator* evaluator)
        {
            stmtEvaluator = evaluator;
        }

        void SuperCallHandler::setObjectEvaluator(interfaces::IObjectEvaluator* evaluator)
        {
            objEvaluator = evaluator;
        }

        Value SuperCallHandler::evaluateSuperConstructorCall(SuperConstructorCallNode* node)
        {
            // Get current instance - super() can only be called in a constructor
            auto currentInstance = context->getCurrentInstance();
            if (!currentInstance) {
                throw UndefinedException(
                    "super() can only be called within a constructor",
                    node->getLocation());
            }

            // Use currentConstructorClass to get the right parent
            // If currentConstructorClass is set, use it; otherwise fall back to instance's class
            auto currentClass = context->getCurrentConstructorClass();
            if (!currentClass) {
                currentClass = currentInstance->getClassDefinition();
            }

            if (!currentClass->hasParentClass()) {
                throw UndefinedException(
                    "Class '" + currentClass->getName() +
                    "' has no parent class, cannot call super()",
                    node->getLocation());
            }

            auto parentClass = currentClass->getParentClass();
            if (!parentClass) {
                throw UndefinedException(
                    "Parent class not found for super() call",
                    node->getLocation());
            }

            // Evaluate constructor arguments
            std::vector<Value> argValues;
            for (const auto& arg : node->getArguments()) {
                argValues.push_back(exprEvaluator->evaluate(arg.get()));
            }

            // Find matching parent constructor
            auto parentConstructor = parentClass->findConstructor(argValues.size());
            if (!parentConstructor) {
                throw UndefinedException(
                    "No matching constructor in parent class '" + parentClass->getName() +
                    "' with " + std::to_string(argValues.size()) + " parameter(s)",
                    node->getLocation());
            }

            // Execute parent constructor in the context of current instance
            if (objEvaluator) {
                // Create new scope for parent constructor execution
                context->getEnvironment()->enterScope();

                // Get generic type bindings from context
                auto genericBindings = context->getGenericTypeBindings();

                // Use ParameterBinder with full type information if available
                if (parentConstructor->hasParametersWithTypes()) {
                    utils::ParameterBinder::bindAndValidateParameters(
                        parentConstructor->getParametersWithTypes(),
                        argValues,
                        "constructor for parent class '" + parentClass->getName() + "'",
                        context->getEnvironment(),
                        genericBindings,  // Pass generic bindings for type resolution
                        node->getLocation()
                    );
                } else {
                    // Fallback to old format
                    utils::ParameterBinder::bindAndValidateParameters(
                        parentConstructor->getParameters(),
                        argValues,
                        "constructor for parent class '" + parentClass->getName() + "'",
                        context->getEnvironment(),
                        node->getLocation()
                    );
                }

                // Execute parent's super initializer first (if it has one)
                if (parentConstructor->hasSuperInitializer()) {
                    auto parentSuperInit = parentConstructor->getSuperInitializer();
                    if (parentSuperInit) {
                        // Set currentConstructorClass to parent so super() knows which class's constructor is executing
                        auto prevConstructorClass = context->getCurrentConstructorClass();
                        context->setCurrentConstructorClass(parentClass);

                        // We're already in super initializer context, so this will work
                        exprEvaluator->evaluate(static_cast<ASTNode*>(parentSuperInit));

                        context->setCurrentConstructorClass(prevConstructorClass);
                    }
                }

                // Execute parent constructor body
                Value result = std::monostate{};
                if (parentConstructor->getBody()) {
                    // Set currentConstructorClass to parent so nested super() calls work correctly
                    auto prevConstructorClass = context->getCurrentConstructorClass();
                    context->setCurrentConstructorClass(parentClass);

                    result = stmtEvaluator->evaluate(parentConstructor->getBody());

                    // Restore previous constructor class
                    context->setCurrentConstructorClass(prevConstructorClass);
                }

                context->getEnvironment()->exitScope();
                return result;
            }

            throw UndefinedException(
                "Object evaluator not available for super() call",
                node->getLocation());
        }

        Value SuperCallHandler::evaluateSuperMethodCall(SuperMethodCallNode* node)
        {
            // Get current instance - super.method() can only be called in instance methods
            auto currentInstance = context->getCurrentInstance();
            if (!currentInstance) {
                throw UndefinedException(
                    "super." + node->getMethodName() + "() can only be called within an instance method",
                    node->getLocation());
            }

            // IMPORTANT: Use calling class stack to determine which class's method we're executing
            // This prevents infinite recursion in multi-level inheritance (e.g., AdvancedService -> DerivedService -> BaseService)
            // Using currentInstance->getClassDefinition() would always give us the runtime class (AdvancedService),
            // causing DerivedService.method() to incorrectly call itself instead of BaseService.method()
            std::string currentClassName = context->getCurrentCallingClass();
            if (currentClassName.empty()) {
                // ERROR: Calling class stack is empty! This should never happen in normal method execution
                // The fallback is unsafe because it gives us the runtime type, which can cause infinite recursion
                throw UndefinedException(
                    "super." + node->getMethodName() + "() called without proper method context. "
                    "Calling class stack is empty - this indicates a compiler bug or corrupted execution state.",
                    node->getLocation());
            }

            auto env = context->getEnvironment();
            auto currentClass = env->findClass(currentClassName);
            if (!currentClass) {
                throw UndefinedException(
                    "Current class '" + currentClassName + "' not found for super." + node->getMethodName() + "() call",
                    node->getLocation());
            }

            // Detect circular inheritance: walk up the parent chain and ensure we don't loop
            {
                std::unordered_set<std::string> visitedClasses;
                auto checkClass = currentClass;
                while (checkClass) {
                    if (visitedClasses.count(checkClass->getName())) {
                        throw UndefinedException(
                            "Circular inheritance detected for class '" + currentClassName + "' - "
                            "cannot resolve super." + node->getMethodName() + "() call",
                            node->getLocation());
                    }
                    visitedClasses.insert(checkClass->getName());

                    if (!checkClass->hasParentClass()) {
                        break;
                    }
                    checkClass = checkClass->getParentClass();
                }
            }

            if (!currentClass->hasParentClass()) {
                throw UndefinedException(
                    "Class '" + currentClass->getName() +
                    "' has no parent class, cannot call super." + node->getMethodName() + "()",
                    node->getLocation());
            }

            auto parentClass = currentClass->getParentClass();
            if (!parentClass) {
                throw UndefinedException(
                    "Parent class not found for super." + node->getMethodName() + "() call",
                    node->getLocation());
            }

            // Evaluate method arguments
            std::vector<Value> argValues;
            for (const auto& arg : node->getArguments()) {
                argValues.push_back(exprEvaluator->evaluate(arg.get()));
            }

            // Find method in parent class (not in current class - that's the override)
            auto parentMethod = parentClass->findMethod(node->getMethodName(), argValues.size());
            if (!parentMethod) {
                throw UndefinedException(
                    "Method '" + node->getMethodName() + "' with " +
                    std::to_string(argValues.size()) + " parameter(s) not found in parent class '" +
                    parentClass->getName() + "'",
                    node->getLocation());
            }

            // Call parent method using object evaluator
            if (objEvaluator) {
                // Create new scope for method execution
                context->getEnvironment()->enterScope();

                // Bind 'this' to current instance
                auto thisVarDef = std::make_shared<VariableDefinition>(
                    "this",
                    ValueType::OBJECT,
                    currentInstance,
                    true  // 'this' is effectively final
                );
                context->getEnvironment()->declareVariable("this", thisVarDef);

                // Bind method parameters
                const auto& params = parentMethod->getParameters();
                for (size_t i = 0; i < params.size() && i < argValues.size(); ++i) {
                    auto varDef = std::make_shared<VariableDefinition>(
                        params[i].first,
                        params[i].second.basicType,  // Extract ValueType from ParameterType
                        argValues[i],
                        false  // parameters are not final
                    );
                    context->getEnvironment()->declareVariable(params[i].first, varDef);
                }

                // Push parent class onto calling class stack for correct super resolution
                context->pushCallingClass(parentClass->getName());

                // Execute parent method body
                Value result = std::monostate{};
                if (parentMethod->getBody()) {
                    try {
                        result = stmtEvaluator->evaluate(parentMethod->getBody());

                        // Check if method returned a value
                        if (context->shouldReturn()) {
                            result = context->getReturnValue();
                            context->setReturned(false);  // Reset return flag
                        }
                    } catch (const ReturnException& e) {
                        // Parent method returned - extract the return value
                        // DO NOT re-throw - super.method() calls should return normally
                        result = e.returnValue;
                        context->setReturned(false);  // Reset return flag
                    }
                }

                // Pop calling class stack
                context->popCallingClass();
                context->getEnvironment()->exitScope();

                // Wrap in Promise if parent method is async
                if (parentMethod->getIsAsync()) {
                    auto promise = std::make_shared<value::PromiseValue>(result);
                    return promise;
                }

                return result;
            }

            throw UndefinedException(
                "Object evaluator not available for super." + node->getMethodName() + "() call",
                node->getLocation());
        }
    }
}
