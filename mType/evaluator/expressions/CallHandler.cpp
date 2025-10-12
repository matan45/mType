#include "CallHandler.hpp"
#include "../ExpressionEvaluator.hpp"
#include "../StatementEvaluator.hpp"
#include "../ObjectEvaluator.hpp"
#include "../utils/ScopeGuard.hpp"
#include "../utils/ParameterBinder.hpp"
#include "../utils/AsyncReturnGuard.hpp"
#include "../validation/TypeValidator.hpp"
#include "../../errors/UndefinedException.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/ReturnException.hpp"
#include "../../value/LambdaValue.hpp"
#include "../../value/PromiseValue.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/global/FunctionDefinition.hpp"
#include "../../environment/manager/Scope.hpp"
#include "../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../ast/nodes/expressions/LambdaInterfaceInvocationNode.hpp"

using namespace errors;
using namespace evaluator::utils;
using namespace environment::manager;

namespace evaluator
{
    namespace expressions
    {
        CallHandler::CallHandler(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr),
              stmtEvaluator(nullptr), objEvaluator(nullptr)
        {
        }

        void CallHandler::setExpressionEvaluator(ExpressionEvaluator* evaluator)
        {
            exprEvaluator = evaluator;
        }

        void CallHandler::setStatementEvaluator(StatementEvaluator* evaluator)
        {
            stmtEvaluator = evaluator;
        }

        void CallHandler::setObjectEvaluator(ObjectEvaluator* evaluator)
        {
            objEvaluator = evaluator;
        }

        Value CallHandler::evaluateFunctionCall(FunctionCallNode* node)
        {
            auto env = context->getEnvironment();

            // First check if it's a native function
            auto nativeRegistry = env->getNativeRegistry();
            if (nativeRegistry->hasNativeFunction(node->getFunctionName()))
            {
                auto nativeFunc = nativeRegistry->findNativeFunction(node->getFunctionName());

                // Evaluate arguments
                std::vector<Value> args;
                for (auto& argNode : node->getArguments())
                {
                    args.push_back(exprEvaluator->evaluate(argNode.get()));
                }

                // Call native function
                return nativeFunc(args);
            }

            // Check if this is a qualified call (contains ::)
            std::string functionName = node->getFunctionName();
            if (functionName.find("::") != std::string::npos)
            {
                // Parse the qualified name into parts
                std::vector<std::string> parts;
                size_t start = 0;
                size_t pos = 0;
                while ((pos = functionName.find("::", start)) != std::string::npos)
                {
                    parts.push_back(functionName.substr(start, pos - start));
                    start = pos + 2;
                }
                parts.push_back(functionName.substr(start));

                // Treat qualified calls as static method calls: ClassName::methodName
                if (parts.size() == 2)
                {
                    std::string className = parts[0];
                    std::string methodName = parts[1];

                    // Handle 'this::methodName' syntax
                    if (className == "this")
                    {
                        auto currentInstance = context->getCurrentInstance();
                        if (currentInstance)
                        {
                            className = currentInstance->getClassDefinition()->getClassName();
                        }
                        else
                        {
                            auto currentClassVar = env->findVariable("__current_class_name__");
                            if (currentClassVar)
                            {
                                auto currentClassValue = currentClassVar->getValue();
                                if (std::holds_alternative<std::string>(currentClassValue))
                                {
                                    className = std::get<std::string>(currentClassValue);
                                }
                                else
                                {
                                    throw UndefinedException("Cannot determine class context for 'this' qualifier",
                                                             node->getLocation());
                                }
                            }
                            else
                            {
                                throw UndefinedException("'this' qualifier can only be used within class methods",
                                                         node->getLocation());
                            }
                        }
                    }

                    auto classRegistry = env->getClassRegistry();
                    auto classDef = classRegistry->findItem(className);

                    if (classDef)
                    {
                        // Found a class - try to call static method
                        std::vector<Value> args;
                        for (auto& argNode : node->getArguments())
                        {
                            args.push_back(exprEvaluator->evaluate(argNode.get()));
                        }

                        // Look for static method
                        auto method = classDef->getMethod(methodName);
                        if (method && method->isStatic())
                        {
                            if (objEvaluator)
                            {
                                return objEvaluator->callStaticMethod(className, methodName, args, node->getLocation());
                            }
                            else
                            {
                                throw UndefinedException("Object evaluator not available for static method call",
                                                         node->getLocation());
                            }
                        }
                        else
                        {
                            throw UndefinedException(
                                "Static method '" + methodName + "' not found in class '" + className + "'",
                                node->getLocation());
                        }
                    }
                    else
                    {
                        throw UndefinedException(
                            "Class '" + className + "' not found for qualified call '" + functionName + "'",
                            node->getLocation());
                    }
                }
                else
                {
                    throw UndefinedException("Complex qualified function calls not supported: '" + functionName + "'",
                                             node->getLocation());
                }
            }

            // Check if we're in a method context and this could be a method call
            auto currentInstance = context->getCurrentInstance();
            if (currentInstance)
            {
                auto method = currentInstance->getClassDefinition()->getMethod(node->getFunctionName());
                if (method && !method->isStatic())
                {
                    // This is a method call on the current instance
                    std::vector<Value> args;
                    for (auto& argNode : node->getArguments())
                    {
                        args.push_back(exprEvaluator->evaluate(argNode.get()));
                    }

                    if (objEvaluator)
                    {
                        return objEvaluator->callMethod(currentInstance, node->getFunctionName(), args,
                                                        node->getLocation());
                    }
                    else
                    {
                        throw UndefinedException("Object evaluator not available for method call", node->getLocation());
                    }
                }
            }

            // Check if we're in a static method context and this could be a static method call
            auto currentClassVar = env->findVariable("__current_class_name__");
            if (currentClassVar)
            {
                auto currentClassValue = currentClassVar->getValue();
                if (std::holds_alternative<std::string>(currentClassValue))
                {
                    std::string className = std::get<std::string>(currentClassValue);

                    auto classDef = env->findClass(className);
                    if (classDef)
                    {
                        auto method = classDef->getMethod(node->getFunctionName());
                        if (method && method->isStatic())
                        {
                            // This is a static method call on the current class
                            std::vector<Value> args;
                            for (auto& argNode : node->getArguments())
                            {
                                args.push_back(exprEvaluator->evaluate(argNode.get()));
                            }

                            if (objEvaluator)
                            {
                                return objEvaluator->callStaticMethod(className, node->getFunctionName(), args,
                                                                      node->getLocation());
                            }
                            else
                            {
                                throw UndefinedException("Object evaluator not available for static method call",
                                                         node->getLocation());
                            }
                        }
                    }
                }
            }

            // Check for user-defined function
            auto funcDef = env->findFunction(node->getFunctionName());

            if (!funcDef)
            {
                throw UndefinedException("Undefined function: " + node->getFunctionName(), node->getLocation());
            }

            // Handle generic function calls
            auto functionToCall = funcDef;
            if (node->hasGenericTypeArguments())
            {
                // Validate that the function is actually generic
                if (!funcDef->hasGenericInformation())
                {
                    throw TypeException(
                        "Function '" + node->getFunctionName() + "' is not generic but generic type arguments were provided",
                        node->getLocation());
                }

                // Validate that the number of type arguments matches the number of type parameters
                const auto& genericTypeParams = funcDef->getGenericTypeParameters();
                const auto& genericTypeArgs = node->getGenericTypeArguments();
                if (genericTypeArgs.size() != genericTypeParams.size())
                {
                    throw TypeException(
                        "Function '" + node->getFunctionName() + "' expects " +
                        std::to_string(genericTypeParams.size()) + " type argument(s) but got " +
                        std::to_string(genericTypeArgs.size()),
                        node->getLocation());
                }
            }

            // Evaluate arguments
            std::vector<Value> args;
            for (auto& argNode : node->getArguments())
            {
                args.push_back(exprEvaluator->evaluate(argNode.get()));
            }

            // Convert lambda arguments to interface implementations if needed
            // This must happen BEFORE parameter binding to allow lambdas to be passed
            // as interface-type parameters (similar to variable assignment)
            const auto& params = funcDef->getParameters();
            for (size_t i = 0; i < args.size() && i < params.size(); ++i)
            {
                const auto& param = params[i];
                Value& arg = args[i];

                // Check if argument is a lambda and parameter expects an interface
                if (std::holds_alternative<std::shared_ptr<value::LambdaValue>>(arg) &&
                    param.second.isInterface())
                {
                    std::string interfaceName = param.second.getInterfaceName();

                    if (stmtEvaluator)
                    {
                        try
                        {
                            arg = stmtEvaluator->convertLambdaToInterface(arg, interfaceName, node->getLocation());
                        }
                        catch (...)
                        {
                            // Keep original lambda value and let parameter binding handle the error
                        }
                    }
                }
            }

            // Use ScopeGuard for automatic scope management
            {
                ScopeGuard scope(env, node->getFunctionName(), ScopeType::FUNCTION);

                // Set up generic type bindings if this is a generic function call
                auto previousGenericBindings = context->getGenericTypeBindings();
                if (node->hasGenericTypeArguments() && functionToCall->hasGenericInformation())
                {
                    const auto& genericTypeParams = functionToCall->getGenericTypeParameters();
                    const auto& genericTypeArgs = node->getGenericTypeArguments();
                    std::unordered_map<std::string, std::string> typeBindings;

                    for (size_t i = 0; i < genericTypeParams.size() && i < genericTypeArgs.size(); ++i)
                    {
                        typeBindings[genericTypeParams[i].name] = genericTypeArgs[i];
                    }

                    context->setGenericTypeBindings(typeBindings);
                }

                // Validate and bind parameters (includes comprehensive type checking)
                ParameterBinder::bindAndValidateParameters(
                    funcDef->getParameters(),
                    args,
                    "function '" + node->getFunctionName() + "'",
                    env,
                    node->getLocation()
                );

                // Execute function body
                Value result = std::monostate{};
                try
                {
                    if (stmtEvaluator)
                    {
                        stmtEvaluator->evaluate(funcDef->getBody().get());

                        // Get return value if function returned
                        if (context->shouldReturn())
                        {
                            result = context->getReturnValue();
                            context->setReturned(false);

                            // Check for lambda-to-interface conversion before validation
                            if (std::holds_alternative<std::shared_ptr<value::LambdaValue>>(result) &&
                                funcDef->getReturnType() == ValueType::OBJECT)
                            {
                                if (stmtEvaluator)
                                {
                                    std::string expectedTypeName = funcDef->getReturnClassName();
                                    if (!expectedTypeName.empty())
                                    {
                                        try
                                        {
                                            result = stmtEvaluator->convertLambdaToInterface(result, expectedTypeName);
                                        }
                                        catch (...)
                                        {
                                            // Keep original lambda value and let validation handle it
                                        }
                                    }
                                }
                            }

                            // Validate return type
                            validation::TypeValidator::validateFunctionReturn(
                                funcDef->getReturnType(), result, node->getFunctionName(),
                                node->getLocation());
                        }
                    }
                    else
                    {
                        throw UndefinedException("Statement evaluator not available for function execution",
                                                 node->getLocation());
                    }
                }
                catch (const ReturnException& e)
                {
                    // Handle return statement
                    context->setReturned(false);

                    // RAII guard ensures promise wrapping even if exception occurs
                    utils::AsyncReturnGuard asyncGuard(funcDef->getIsAsync());

                    // Check for lambda-to-interface conversion
                    Value returnValue = e.returnValue;
                    if (std::holds_alternative<std::shared_ptr<LambdaValue>>(returnValue) &&
                        funcDef->getReturnType() == ValueType::OBJECT)
                    {
                        if (stmtEvaluator)
                        {
                            std::string expectedTypeName = funcDef->getReturnClassName();
                            if (!expectedTypeName.empty())
                            {
                                try
                                {
                                    returnValue = stmtEvaluator->
                                        convertLambdaToInterface(returnValue, expectedTypeName);
                                }
                                catch (...)
                                {
                                    // Keep original lambda value
                                }
                            }
                        }
                    }

                    // Validate return type
                    validation::TypeValidator::validateFunctionReturn(
                        funcDef->getReturnType(), returnValue, node->getFunctionName(),
                        node->getLocation());

                    // Restore previous generic type bindings before returning
                    if (node->hasGenericTypeArguments() && functionToCall->hasGenericInformation())
                    {
                        context->setGenericTypeBindings(previousGenericBindings);
                    }

                    // Wrap in Promise if async function (exception-safe via RAII)
                    return asyncGuard.wrapIfNeeded(returnValue);
                }
                catch (...)
                {
                    // Restore previous generic type bindings on exception
                    if (node->hasGenericTypeArguments() && functionToCall->hasGenericInformation())
                    {
                        context->setGenericTypeBindings(previousGenericBindings);
                    }
                    throw;
                }

                // Restore previous generic type bindings
                if (node->hasGenericTypeArguments() && functionToCall->hasGenericInformation())
                {
                    context->setGenericTypeBindings(previousGenericBindings);
                }

                // Wrap in Promise if async function (exception-safe via RAII)
                utils::AsyncReturnGuard asyncGuard(funcDef->getIsAsync());
                return asyncGuard.wrapIfNeeded(result);
            }
        }

        Value CallHandler::evaluateMethodCall(MethodCallNode* node)
        {
            // Delegate ALL method calls to ObjectEvaluator which handles both static and instance calls
            if (objEvaluator)
            {
                return objEvaluator->evaluateMethodCallNode(node);
            }

            throw UndefinedException("Object evaluator not available for method call", node->getLocation());
        }

        Value CallHandler::evaluateLambdaInterfaceInvocation(LambdaInterfaceInvocationNode* node)
        {
            // Evaluate the lambda/object
            Value lambdaValue = exprEvaluator->evaluate(node->getLambdaNode().get());

            // Check if it's a lambda wrapper object
            if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(lambdaValue))
            {
                auto objectInstance = std::get<std::shared_ptr<ObjectInstance>>(lambdaValue);

                // Evaluate arguments
                std::vector<Value> args;
                for (auto& argNode : node->getArguments())
                {
                    args.push_back(exprEvaluator->evaluate(argNode.get()));
                }

                // Call the interface method
                if (objEvaluator)
                {
                    return objEvaluator->callMethod(objectInstance, node->getMethodName(), args, node->getLocation());
                }
                else
                {
                    throw UndefinedException("Object evaluator not available for lambda interface invocation",
                                             node->getLocation());
                }
            }
            else
            {
                throw TypeException("Lambda interface invocation requires lambda wrapper object", node->getLocation());
            }
        }
    }
}
