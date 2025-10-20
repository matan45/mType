#include "GenericInstantiationHandler.hpp"
#include "../ExpressionEvaluator.hpp"
#include "../StatementEvaluator.hpp"
#include "../ObjectEvaluator.hpp"
#include "../utils/GenericTypeManager.hpp"
#include "../utils/ParameterBinder.hpp"
#include "../utils/ScopeGuard.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/UndefinedException.hpp"
#include "../../environment/manager/Scope.hpp"
#include "../validation/AccessValidator.hpp"
#include "../base/AccessContext.hpp"

using namespace errors;
using namespace runtimeTypes::klass;

namespace evaluator {
namespace objects {

    Value GenericInstantiationHandler::evaluateNew(NewNode* node)
    {
        std::vector<Value> args = evaluateArgumentList(node->getArguments());
        std::string className = node->getClassName();

        // Resolve type parameters from context
        std::string resolvedClassName = resolveClassName(className);

        // Find or instantiate the class
        auto classDef = findOrInstantiateClass(resolvedClassName, node);
        if (!classDef)
        {
            throw UndefinedException("Class '" + resolvedClassName + "' not found");
        }

        // Extract generic type bindings for this instance
        auto genericTypeBindings = extractGenericTypeBindings(resolvedClassName);

        // Create instance using ObjectEvaluator's helper method
        auto instance = objEvaluator->createInstanceWithTypeBindings(resolvedClassName, args, genericTypeBindings);

        // Get environment for constructor execution
        auto env = context->getEnvironment();

        // Execute constructor if it exists
        if (classDef && !classDef->getConstructors().empty())
        {
            auto constructor = classDef->findConstructor(args.size());
            if (!constructor)
            {
                // Class has constructors but none match the provided arguments
                throw TypeException("No matching constructor for class '" + node->getClassName() +
                                    "' with " + std::to_string(args.size()) + " argument(s)", node->getLocation());
            }

            // ACCESS CONTROL: Validate constructor access permissions
            auto callingClassName = context->getCurrentCallingClass();
            auto callingClassDef = callingClassName.empty() ? nullptr : env->findClass(callingClassName);
            auto accessContext = base::AccessContext::forStaticAccess(
                callingClassName,
                classDef,
                node->getLocation(),
                callingClassDef
            );
            validation::AccessValidator::validateConstructorAccess(accessContext, *constructor);

            if (constructor && constructor->getBody())
            {
                // Set the current instance for constructor execution
                auto prevInstance = context->getCurrentInstance();
                context->setCurrentInstance(instance);

                // Set generic type bindings for this instance so parameter validation can resolve generic types
                auto prevGenericBindings = context->getGenericTypeBindings();
                context->setGenericTypeBindings(genericTypeBindings);

                // Temporarily clear static method context for constructor execution
                // Constructors always run in instance context regardless of where they're called from
                bool wasInStaticMethod = context->isInStaticMethodContext();
                context->setInStaticMethod(false);

                // Use ScopeGuard for automatic scope management
                {
                    utils::ScopeGuard scope(env, "constructor", environment::manager::ScopeType::FUNCTION);

                    try
                    {
                        // Use ParameterBinder with full type information if available
                        if (constructor->hasParametersWithTypes()) {
                            utils::ParameterBinder::bindAndValidateParameters(
                                constructor->getParametersWithTypes(),
                                args,
                                "constructor for class '" + node->getClassName() + "'",
                                env,
                                genericTypeBindings,  // Pass generic bindings for type resolution
                                node->getLocation()
                            );
                        } else {
                            // Fallback to old format
                            utils::ParameterBinder::bindAndValidateParameters(
                                constructor->getParameters(),
                                args,
                                "constructor for class '" + node->getClassName() + "'",
                                env,
                                node->getLocation()
                            );
                        }

                        // Execute super initializer first (if present)
                        if (constructor->hasSuperInitializer())
                        {
                            auto superInit = constructor->getSuperInitializer();
                            if (superInit && exprEvaluator)
                            {
                                // Set currentConstructorClass before super call
                                auto prevConstructorClass = context->getCurrentConstructorClass();
                                context->setCurrentConstructorClass(classDef);

                                // Mark that we're in super initializer context (allowed)
                                context->setInSuperInitializerContext(true);
                                exprEvaluator->evaluate(static_cast<ASTNode*>(superInit));
                                context->setInSuperInitializerContext(false);

                                context->setCurrentConstructorClass(prevConstructorClass);
                            }
                        }

                        // Execute constructor body
                        if (stmtEvaluator)
                        {
                            auto bodyPtr = constructor->getBody();
                            if (bodyPtr)
                            {
                                // Set currentConstructorClass so super() knows which class's constructor is executing
                                auto prevConstructorClass = context->getCurrentConstructorClass();
                                context->setCurrentConstructorClass(classDef);

                                stmtEvaluator->evaluate(bodyPtr);

                                // Restore previous constructor class
                                context->setCurrentConstructorClass(prevConstructorClass);
                            }
                        }
                    }
                    catch (...)
                    {
                        context->setCurrentInstance(prevInstance);
                        context->setInStaticMethod(wasInStaticMethod);
                        context->setGenericTypeBindings(prevGenericBindings);
                        throw;
                    }
                    // Scope automatically exits via RAII
                }
                context->setCurrentInstance(prevInstance);
                context->setInStaticMethod(wasInStaticMethod);
                context->setGenericTypeBindings(prevGenericBindings);
            }
        }

        // Explicitly convert to Value type
        Value result = Value(instance);

        // Debug: Check what type we're actually returning
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(result))
        {
            // Good - we have an object
        }
        else if (std::holds_alternative<std::monostate>(result))
        {
            // Bad - we have void
            throw TypeException("Object creation returned void instead of instance");
        }
        else
        {
            throw TypeException("Object creation returned unexpected type");
        }

        return result;
    }

    std::vector<Value> GenericInstantiationHandler::evaluateArgumentList(const std::vector<std::unique_ptr<ast::ASTNode>>& args)
    {
        std::vector<Value> result;
        if (exprEvaluator)
        {
            for (const auto& arg : args)
            {
                result.push_back(exprEvaluator->evaluate(arg.get()));
            }
        }
        return result;
    }

    std::string GenericInstantiationHandler::resolveTypeParameterFromContext(const std::string& typeParam)
    {
        // Get current instance to check for generic type bindings
        auto currentInstance = context->getCurrentInstance();
        if (currentInstance)
        {
            const auto& typeBindings = currentInstance->getGenericTypeBindings();
            auto it = typeBindings.find(typeParam);
            if (it != typeBindings.end())
            {
                return it->second;
            }
        }
        return typeParam; // Return original if not found
    }

    // ========== Helper Methods for evaluateNew Refactoring ==========

    std::string GenericInstantiationHandler::resolveClassName(const std::string& className)
    {
        if (!utils::GenericTypeManager::isGenericInstantiation(className))
        {
            return className;
        }

        auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(className);
        std::vector<std::string> resolvedTypeArguments;
        bool hasResolvedParams = false;

        for (const std::string& typeArg : typeArguments)
        {
            std::string resolvedType = resolveTypeParameterFromContext(typeArg);
            resolvedTypeArguments.push_back(resolvedType);
            if (resolvedType != typeArg)
            {
                hasResolvedParams = true;
            }
        }

        if (!hasResolvedParams)
        {
            return className;
        }

        // Construct resolved class name
        std::string resolvedClassName = baseName + "<" + resolvedTypeArguments[0];
        for (size_t i = 1; i < resolvedTypeArguments.size(); ++i)
        {
            resolvedClassName += "," + resolvedTypeArguments[i];
        }
        resolvedClassName += ">";
        return resolvedClassName;
    }

    std::shared_ptr<runtimeTypes::klass::ClassDefinition>
    GenericInstantiationHandler::findOrInstantiateClass(const std::string& className, const NewNode* node)
    {
        auto env = context->getEnvironment();

        // Check if this is a generic instantiation
        if (!utils::GenericTypeManager::isGenericInstantiation(className))
        {
            return env->findClass(className);
        }

        // Parse generic instantiation
        auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(className);

        // Try to find already instantiated class
        auto instantiatedClass = env->findClass(className);
        if (instantiatedClass)
        {
            return instantiatedClass;
        }

        // Find the generic class template
        auto genericClass = env->findClass(baseName);
        if (!genericClass)
        {
            throw UndefinedException("Generic class '" + baseName + "' not found");
        }

        if (!genericClass->isGeneric())
        {
            throw TypeException("Class '" + baseName + "' is not generic but used with type arguments",
                               node->getLocation());
        }

        // Validate type arguments
        if (!utils::GenericTypeManager::validateTypeArguments(genericClass, typeArguments))
        {
            throw TypeException("Invalid type arguments for generic class '" + baseName + "'",
                               node->getLocation());
        }

        // Create instantiated class
        auto classDef = utils::GenericTypeManager::instantiateGenericClass(genericClass, typeArguments);
        if (!classDef)
        {
            throw TypeException("Failed to instantiate generic class '" + className + "'",
                               node->getLocation());
        }

        // Register the instantiated class for future use
        env->registerClass(className, classDef);
        return classDef;
    }

    std::unordered_map<std::string, std::string>
    GenericInstantiationHandler::extractGenericTypeBindings(const std::string& classNameForInstance)
    {
        std::unordered_map<std::string, std::string> genericTypeBindings;

        if (!utils::GenericTypeManager::isGenericInstantiation(classNameForInstance))
        {
            return genericTypeBindings;
        }

        auto env = context->getEnvironment();
        auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(classNameForInstance);

        // Look up the generic template class
        auto templateClassDef = env->findClass(baseName);
        if (!templateClassDef || !templateClassDef->isGeneric())
        {
            return genericTypeBindings;
        }

        const auto& genericParams = templateClassDef->getGenericParameters();

        // Map each generic parameter to its concrete type
        for (size_t i = 0; i < genericParams.size() && i < typeArguments.size(); ++i)
        {
            std::string resolvedTypeArg = typeArguments[i];

            // Try to resolve from context's type bindings (for generic methods)
            const auto& contextBindings = context->getGenericTypeBindings();
            auto it = contextBindings.find(typeArguments[i]);
            if (it != contextBindings.end())
            {
                resolvedTypeArg = it->second;
            }
            else
            {
                // Try to resolve from current instance's type bindings (for generic classes)
                auto currentInstance = context->getCurrentInstance();
                if (currentInstance)
                {
                    const auto& instanceBindings = currentInstance->getGenericTypeBindings();
                    auto instIt = instanceBindings.find(typeArguments[i]);
                    if (instIt != instanceBindings.end())
                    {
                        resolvedTypeArg = instIt->second;
                    }
                }
            }

            genericTypeBindings[genericParams[i].name] = resolvedTypeArg;
        }

        return genericTypeBindings;
    }

} // namespace objects
} // namespace evaluator
