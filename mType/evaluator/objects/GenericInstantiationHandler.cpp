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

using namespace errors;
using namespace runtimeTypes::klass;

namespace evaluator {
namespace objects {

    Value GenericInstantiationHandler::evaluateNew(NewNode* node)
    {
        std::vector<Value> args = evaluateArgumentList(node->getArguments());
        std::string className = node->getClassName();

        // Handle regular class instantiation
        std::shared_ptr<ClassDefinition> classDef;
        auto env = context->getEnvironment();

        // Try to resolve type parameters from current object context first
        std::string resolvedClassName = className;
        if (utils::GenericTypeManager::isGenericInstantiation(className))
        {
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

            // If we resolved any type parameters, construct the resolved class name
            if (hasResolvedParams)
            {
                resolvedClassName = baseName + "<" + resolvedTypeArguments[0];
                for (size_t i = 1; i < resolvedTypeArguments.size(); ++i)
                {
                    resolvedClassName += "," + resolvedTypeArguments[i];
                }
                resolvedClassName += ">";
            }
        }

        // Check if we should use the resolved class name for direct instantiation
        if (resolvedClassName != className)
        {
            // We resolved type parameters - try to find the instantiated class directly
            auto instantiatedClass = env->findClass(resolvedClassName);
            if (instantiatedClass)
            {
                classDef = instantiatedClass;
            }
            else
            {
                className = resolvedClassName; // Use resolved name for generic instantiation
            }
        }

        // Check if this is a generic instantiation (only if we haven't already found instantiated class)
        if (!classDef && utils::GenericTypeManager::isGenericInstantiation(className))
        {
            // Parse generic instantiation
            auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(className);

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
            classDef = utils::GenericTypeManager::instantiateGenericClass(genericClass, typeArguments);
            if (!classDef)
            {
                throw TypeException("Failed to instantiate generic class '" + className + "'",
                                   node->getLocation());
            }

            // Register the instantiated class for future use
            env->registerClass(className, classDef);
        }
        else
        {
            // Regular non-generic class - use resolved class name if available
            std::string classNameToLookup = (resolvedClassName != className) ? resolvedClassName : className;
            classDef = env->findClass(classNameToLookup);
        }

        if (!classDef)
        {
            std::string classNameToReport = (resolvedClassName != className) ? resolvedClassName : className;
            throw UndefinedException("Class '" + classNameToReport + "' not found");
        }

        std::string classNameForInstance = (resolvedClassName != className) ? resolvedClassName : className;

        // Extract generic type bindings for this instance
        std::unordered_map<std::string, std::string> genericTypeBindings;
        std::string baseClassName = classNameForInstance;

        if (utils::GenericTypeManager::isGenericInstantiation(classNameForInstance))
        {
            auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(classNameForInstance);

            // Use the base class name for lookup (e.g., "LinkedList" instead of "LinkedList<String>")
            baseClassName = baseName;

            // Look up the generic template class
            auto templateClassDef = env->findClass(baseClassName);

            if (templateClassDef && templateClassDef->isGeneric())
            {
                const auto& genericParams = templateClassDef->getGenericParameters();

                // Map each generic parameter to its concrete type
                for (size_t i = 0; i < genericParams.size() && i < typeArguments.size(); ++i)
                {
                    genericTypeBindings[genericParams[i].name] = typeArguments[i];
                }

                // Use the template class definition for creating the instance
                classDef = templateClassDef;
            }
        }

        // Create instance using ObjectEvaluator's helper method
        auto instance = objEvaluator->createInstanceWithTypeBindings(classNameForInstance, args, genericTypeBindings);

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
            if (constructor && constructor->getBody())
            {
                // Set the current instance for constructor execution
                auto prevInstance = context->getCurrentInstance();
                context->setCurrentInstance(instance);

                // Temporarily clear static method context for constructor execution
                // Constructors always run in instance context regardless of where they're called from
                bool wasInStaticMethod = context->isInStaticMethodContext();
                context->setInStaticMethod(false);

                // Use ScopeGuard for automatic scope management
                {
                    utils::ScopeGuard scope(env, "constructor", environment::manager::ScopeType::FUNCTION);

                    try
                    {
                        // Use ParameterBinder utility to eliminate duplication
                        utils::ParameterBinder::bindAndValidateParameters(
                            constructor->getParameters(),
                            args,
                            "constructor for class '" + node->getClassName() + "'",
                            env,
                            node->getLocation()
                        );

                        // Execute constructor body
                        if (stmtEvaluator)
                        {
                            auto bodyPtr = constructor->getBody();
                            if (bodyPtr)
                            {
                                stmtEvaluator->evaluate(bodyPtr);
                            }
                        }
                    }
                    catch (...)
                    {
                        context->setCurrentInstance(prevInstance);
                        context->setInStaticMethod(wasInStaticMethod);
                        throw;
                    }
                    // Scope automatically exits via RAII
                }
                context->setCurrentInstance(prevInstance);
                context->setInStaticMethod(wasInStaticMethod);
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

} // namespace objects
} // namespace evaluator
