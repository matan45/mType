#include "StaticMemberHandler.hpp"
#include "../StatementEvaluator.hpp"
#include "../utils/ScopeGuard.hpp"
#include "../utils/ParameterBinder.hpp"
#include "../utils/GenericTypeManager.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/UndefinedException.hpp"
#include "../../exception/ReturnException.hpp"
#include <mutex>
#include <unordered_map>

using namespace errors;

namespace evaluator {
namespace objects {

    Value StaticMemberHandler::accessStaticMember(const std::string& className,
                                                  const std::string& memberName)
    {
        auto env = context->getEnvironment();
        return instanceManager->accessStaticMember(className, memberName, env);
    }

    void StaticMemberHandler::assignStaticMember(const std::string& className,
                                                 const std::string& memberName,
                                                 const Value& value)
    {
        auto env = context->getEnvironment();
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

        // Try to find method with exact argument count first
        auto method = classDef->findMethod(methodName, args.size());

        // If not found, try without argument count matching as a fallback
        if (!method)
        {
            method = classDef->getMethod(methodName);
        }

        // Check if method exists and is static
        if (!method)
        {
            throw UndefinedException("Method '" + methodName + "' not found in class '" + className + "'");
        }

        if (!method->isStatic())
        {
            throw UndefinedException("Method '" + methodName + "' in class '" + className + "' is not static");
        }

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
                    context->setCurrentMethod(previousMethod);
                    return result;
                }
                catch (const exception::ReturnException& e)
                {
                    // Handle return exception - extract return value
                    context->setInStaticMethod(previousStaticState);
                    context->setCurrentMethod(previousMethod);
                    context->setReturned(false);
                    return e.returnValue;
                }
                catch (...)
                {
                    // Ensure we restore state even if exception occurs
                    context->setInStaticMethod(previousStaticState);
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
        auto method = classDef->getMethod(methodName);
        if (!method)
        {
            throw UndefinedException("Static method '" + methodName + "' not found in class '" + className + "'");
        }

        if (!method->isStatic())
        {
            throw UndefinedException("Method '" + methodName + "' in class '" + className + "' is not static");
        }

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

                    // Estimate memory usage for the instantiated method
                    size_t estimatedMemory = sizeof(runtimeTypes::klass::MethodDefinition) +
                        signatureKey.size() +
                        (methodToCall->getParameters().size() * sizeof(std::pair<std::string, value::ValueType>));

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

                // Set current method context for generic type resolution
                auto previousMethod = context->getCurrentMethod();
                context->setCurrentMethod(methodToCall);

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
                    context->setCurrentMethod(previousMethod);
                    return result;
                }
                catch (const exception::ReturnException& e)
                {
                    // Handle return exception - extract return value
                    context->setInStaticMethod(previousStaticState);
                    context->setCurrentMethod(previousMethod);
                    context->setReturned(false);
                    return e.returnValue;
                }
                catch (...)
                {
                    // Ensure we restore state even if exception occurs
                    context->setInStaticMethod(previousStaticState);
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

} // namespace objects
} // namespace evaluator
