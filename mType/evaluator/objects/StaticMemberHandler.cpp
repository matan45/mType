#include "StaticMemberHandler.hpp"
#include "../StatementEvaluator.hpp"
#include "../utils/ScopeGuard.hpp"
#include "../utils/ParameterBinder.hpp"
#include "../utils/GenericTypeManager.hpp"
#include "../utils/AsyncReturnGuard.hpp"
#include "../../value/PromiseValue.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/UndefinedException.hpp"
#include "../../errors/ReturnException.hpp"
#include <mutex>
#include <unordered_map>

using namespace errors;

namespace evaluator
{
    namespace objects
    {
        Value StaticMemberHandler::accessStaticMember(const std::string& className,
                                                      const std::string& memberName)
        {
            auto env = context->getEnvironment();

            // Find the class and field
            auto classDef = env->findClass(className);
            if (!classDef)
            {
                throw UndefinedException("Class '" + className + "' not found");
            }

            // Get static fields map and find the field
            const auto& staticFields = classDef->getStaticFields();
            auto fieldIt = staticFields.find(memberName);
            if (fieldIt == staticFields.end())
            {
                throw UndefinedException("Static field '" + memberName + "' not found in class '" + className + "'");
            }
            auto field = fieldIt->second;

            // ACCESS CONTROL: Validate field access permissions
            auto callingClassName = context->getCurrentCallingClass();
            auto callingClassDef = callingClassName.empty() ? nullptr : env->findClass(callingClassName);
            auto accessContext = base::AccessContext::forStaticAccess(
                callingClassName,
                classDef,
                errors::SourceLocation{},
                callingClassDef
            );
            validation::AccessValidator::validateFieldAccess(accessContext, *field);

            return instanceManager->accessStaticMember(className, memberName, env);
        }

        void StaticMemberHandler::assignStaticMember(const std::string& className,
                                                     const std::string& memberName,
                                                     const Value& value)
        {
            auto env = context->getEnvironment();

            // Find the class and field
            auto classDef = env->findClass(className);
            if (!classDef)
            {
                throw UndefinedException("Class '" + className + "' not found");
            }

            // Get static fields map and find the field
            const auto& staticFields = classDef->getStaticFields();
            auto fieldIt = staticFields.find(memberName);
            if (fieldIt == staticFields.end())
            {
                throw UndefinedException("Static field '" + memberName + "' not found in class '" + className + "'");
            }
            auto field = fieldIt->second;

            // ACCESS CONTROL: Validate field access permissions
            auto callingClassName = context->getCurrentCallingClass();
            auto callingClassDef = callingClassName.empty() ? nullptr : env->findClass(callingClassName);
            auto accessContext = base::AccessContext::forStaticAccess(
                callingClassName,
                classDef,
                errors::SourceLocation{},
                callingClassDef
            );
            validation::AccessValidator::validateFieldAccess(accessContext, *field);

            instanceManager->assignStaticMember(className, memberName, value, env);
        }

        Value StaticMemberHandler::callStaticMethod(const std::string& className,
                                                    const std::string& methodName,
                                                    const std::vector<Value>& args,
                                                    const errors::SourceLocation& location)
        {
            auto env = context->getEnvironment();

            // Declare previousMethod for restoration in exception handlers
            auto previousMethod = context->getCurrentMethod();

            // Try to resolve type parameters from current object context first
            std::string resolvedClassName = className;
            auto currentInstance = context->getCurrentInstance();
            if (currentInstance)
            {
                auto instanceClassDef = currentInstance->getClassDefinition();
                if (instanceClassDef)
                {
                    std::string instanceClassName = instanceClassDef->getName(); // e.g., "Set<int>"

                    // Check if the target className contains type parameters that need resolution
                    if (className.find('<') != std::string::npos && className.find('T') != std::string::npos)
                    {
                        if (utils::GenericTypeManager::isGenericInstantiation(instanceClassName))
                        {
                            auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(
                                instanceClassName);

                            // Replace type parameters in className
                            resolvedClassName = className;
                            if (className.find("T") != std::string::npos && !typeArguments.empty())
                            {
                                // Simple T replacement - can be extended for multiple type parameters
                                size_t pos = resolvedClassName.find("T");
                                while (pos != std::string::npos)
                                {
                                    resolvedClassName.replace(pos, 1, typeArguments[0]);
                                    pos = resolvedClassName.find("T", pos + typeArguments[0].length());
                                }
                            }
                        }
                    }
                }
            }

            // Find the class and method
            auto classDef = env->findClass(resolvedClassName);
            if (!classDef)
            {
                throw UndefinedException("Class '" + resolvedClassName + "' not found");
            }

            // Try to find static method with exact argument count first
            auto method = classDef->findStaticMethod(methodName, args.size());

            // If not found, try without argument count matching as a fallback
            if (!method)
            {
                method = classDef->getStaticMethod(methodName);
            }

            // Check if method exists (it's guaranteed to be static if found via getStaticMethod)
            if (!method)
            {
                throw UndefinedException("Static method '" + methodName + "' not found in class '" + className + "'");
            }

            // ACCESS CONTROL: Validate method access permissions
            auto callingClassName = context->getCurrentCallingClass();
            auto callingClassDef = callingClassName.empty() ? nullptr : env->findClass(callingClassName);
            auto accessContext = base::AccessContext::forStaticAccess(
                callingClassName,
                classDef,
                location,
                callingClassDef
            );
            validation::AccessValidator::validateMethodAccess(accessContext, *method);

            // Use ScopeGuard and ParameterBinder utilities
            {
                utils::ScopeGuard scope(env, methodName, environment::manager::ScopeType::FUNCTION);

                try
                {
                    // Use ParameterBinder utility for consistent parameter validation and binding
                    utils::ParameterBinder::bindAndValidateParameters(
                        method->getParameters(),
                        args,
                        "static method '" + className + "::" + methodName + "'",
                        env,
                        location
                    );

                    // Store current class name for static field access
                    auto classNameVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                        "__current_class_name__", ValueType::STRING, className, false
                    );
                    env->declareVariable("__current_class_name__", classNameVar);

                    // Set static method context to prevent instance member access
                    bool previousStaticState = context->isInStaticMethodContext();
                    context->setInStaticMethod(true);

                    // Push calling class onto stack for access control
                    context->pushCallingClass(className);

                    try
                    {
                        // Execute method body (no instance context for static methods)
                        Value result = std::monostate{}; // void default
                        if (method->getBody() && stmtEvaluator)
                        {
                            stmtEvaluator->evaluate(method->getBody());
                        }

                        // Get return value if method returned
                        if (context->shouldReturn())
                        {
                            result = context->getReturnValue();
                            context->setReturned(false);
                        }

                        // Restore previous static method state
                        context->setInStaticMethod(previousStaticState);
                        context->popCallingClass();
                        context->setCurrentMethod(previousMethod);

                        // Wrap in Promise if async method (exception-safe via RAII)
                        utils::AsyncReturnGuard asyncGuard(method->getIsAsync());
                        return asyncGuard.wrapIfNeeded(result);
                    }
                    catch (const ReturnException& e)
                    {
                        // Handle return exception - extract return value
                        context->setInStaticMethod(previousStaticState);
                        context->popCallingClass();
                        context->setCurrentMethod(previousMethod);
                        context->setReturned(false);

                        // Wrap in Promise if async method (exception-safe via RAII)
                        utils::AsyncReturnGuard asyncGuard(method->getIsAsync());
                        return asyncGuard.wrapIfNeeded(e.returnValue);
                    }
                    catch (...)
                    {
                        // Ensure we restore state even if exception occurs
                        context->setInStaticMethod(previousStaticState);
                        context->popCallingClass();
                        context->setCurrentMethod(previousMethod);
                        throw;
                    }
                }
                catch (...)
                {
                    throw; // Re-throw any exceptions that weren't handled by inner try-catch
                }
                // Scope automatically exits via RAII
            }
        }

        Value StaticMemberHandler::callStaticMethodWithGenerics(const std::string& className,
                                                                const std::string& methodName,
                                                                const std::vector<Value>& args,
                                                                const std::vector<std::string>& genericTypeArguments,
                                                                const errors::SourceLocation& location)
        {
            auto env = context->getEnvironment();

            // Get the class definition
            auto classDef = env->getClassRegistry()->findItem(className);
            if (!classDef)
            {
                throw UndefinedException("Class '" + className + "' not found for static method call");
            }

            // Find the static method
            auto method = classDef->getStaticMethod(methodName);
            if (!method)
            {
                throw UndefinedException("Static method '" + methodName + "' not found in class '" + className + "'");
            }

            // ACCESS CONTROL: Validate method access permissions
            auto callingClassName = context->getCurrentCallingClass();
            auto callingClassDef = callingClassName.empty() ? nullptr : env->findClass(callingClassName);
            auto accessContext = base::AccessContext::forStaticAccess(
                callingClassName,
                classDef,
                location,
                callingClassDef
            );
            validation::AccessValidator::validateMethodAccess(accessContext, *method);

            // Handle generic method instantiation
            std::shared_ptr<runtimeTypes::klass::MethodDefinition> methodToCall = method;

            if (!genericTypeArguments.empty())
            {
                // Validate that the method is actually generic
                if (!method->hasGenericInformation())
                {
                    throw TypeException(
                        "Method '" + methodName + "' is not generic but generic type arguments were provided");
                }

                // Validate type arguments
                if (!utils::GenericTypeManager::validateStaticMethodTypeArguments(method, genericTypeArguments))
                {
                    throw TypeException("Invalid type arguments for static generic method '" +
                        className + "::" + methodName + "'");
                }

                // Create a cache key for the instantiated method
                std::string signatureKey = utils::GenericTypeManager::createStaticMethodSignatureKey(
                    className, methodName, genericTypeArguments);

                // Check if we already have this instantiation cached
                static std::mutex staticGenericMethodCacheMutex;
                static std::unordered_map<std::string, std::shared_ptr<runtimeTypes::klass::MethodDefinition>>
                    staticGenericMethodCache;

                {
                    std::lock_guard<std::mutex> lock(staticGenericMethodCacheMutex);

                    auto cacheIt = staticGenericMethodCache.find(signatureKey);
                    if (cacheIt != staticGenericMethodCache.end())
                    {
                        methodToCall = cacheIt->second;
                    }
                    else
                    {
                        // Instantiate the generic method
                        methodToCall = utils::GenericTypeManager::instantiateStaticGenericMethod(
                            method, genericTypeArguments);

                        // Cache the instantiated method
                        staticGenericMethodCache[signatureKey] = methodToCall;
                    }
                }
            }

            // Use ScopeGuard and ParameterBinder utilities
            {
                utils::ScopeGuard scope(env, methodName, environment::manager::ScopeType::FUNCTION);

                try
                {
                    // Use ParameterBinder utility for consistent parameter validation and binding
                    if (methodToCall->hasGenericInformation())
                    {
                        // Use generic-aware parameter binding for instantiated generic methods
                        utils::ParameterBinder::bindAndValidateParameters(
                            methodToCall,
                            args,
                            "static method '" + className + "::" + methodName + "'",
                            env,
                            location
                        );
                    }
                    else
                    {
                        // Use legacy parameter binding for non-generic methods
                        utils::ParameterBinder::bindAndValidateParameters(
                            methodToCall->getParameters(),
                            args,
                            "static method '" + className + "::" + methodName + "'",
                            env,
                            location
                        );
                    }

                    // Store current class name for static field access
                    auto classNameVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                        "__current_class_name__", ValueType::STRING, className, false
                    );
                    env->declareVariable("__current_class_name__", classNameVar);

                    // Set static method context to prevent instance member access
                    bool previousStaticState = context->isInStaticMethodContext();
                    context->setInStaticMethod(true);

                    // Push calling class onto stack for access control
                    context->pushCallingClass(className);

                    // Set current method context for generic type resolution
                    auto previousMethod = context->getCurrentMethod();
                    context->setCurrentMethod(methodToCall);

                    // Set generic type bindings for the method execution
                    // Map generic type parameters (T, U, V) to their concrete types (String, Int, etc.)
                    auto previousGenericBindings = context->getGenericTypeBindings();
                    if (!genericTypeArguments.empty() && methodToCall->hasGenericInformation())
                    {
                        // Use getGenericTypeParameters() to get the <T, U, V> declarations, not function parameters
                        const auto& genericTypeParams = methodToCall->getGenericTypeParameters();
                        std::unordered_map<std::string, std::string> methodTypeBindings;

                        for (size_t i = 0; i < genericTypeParams.size() && i < genericTypeArguments.size(); ++i)
                        {
                            // genericTypeParams is a vector of GenericTypeParameter objects
                            methodTypeBindings[genericTypeParams[i].name] = genericTypeArguments[i];
                        }

                        context->setGenericTypeBindings(methodTypeBindings);
                    }

                    try
                    {
                        // Execute method body (no instance context for static methods)
                        Value result = std::monostate{}; // void default
                        if (methodToCall->getBody() && stmtEvaluator)
                        {
                            stmtEvaluator->evaluate(methodToCall->getBody());
                        }

                        // Get return value if method returned
                        if (context->shouldReturn())
                        {
                            result = context->getReturnValue();
                            context->setReturned(false);
                        }

                        // Restore previous static method state
                        context->setInStaticMethod(previousStaticState);
                        context->popCallingClass();
                        context->setCurrentMethod(previousMethod);
                        context->setGenericTypeBindings(previousGenericBindings);

                        // Wrap in Promise if async method (exception-safe via RAII)
                        utils::AsyncReturnGuard asyncGuard(methodToCall->getIsAsync());
                        return asyncGuard.wrapIfNeeded(result);
                    }
                    catch (const ReturnException& e)
                    {
                        // Handle return exception - extract return value
                        context->setInStaticMethod(previousStaticState);
                        context->popCallingClass();
                        context->setCurrentMethod(previousMethod);
                        context->setGenericTypeBindings(previousGenericBindings);
                        context->setReturned(false);

                        // Wrap in Promise if async method (exception-safe via RAII)
                        utils::AsyncReturnGuard asyncGuard(methodToCall->getIsAsync());
                        return asyncGuard.wrapIfNeeded(e.returnValue);
                    }
                    catch (...)
                    {
                        // Ensure we restore state even if exception occurs
                        context->setInStaticMethod(previousStaticState);
                        context->popCallingClass();
                        context->setCurrentMethod(previousMethod);
                        context->setGenericTypeBindings(previousGenericBindings);
                        throw;
                    }
                }
                catch (...)
                {
                    throw; // Re-throw any exceptions that weren't handled by inner try-catch
                }
                // Scope automatically exits via RAII
            }
        }
    } // namespace objects
} // namespace evaluator
