#include "DeclarationHandler.hpp"
#include "../ExpressionEvaluator.hpp"
#include "../ObjectEvaluator.hpp"
#include "../validation/TypeValidator.hpp"
#include "../utils/GenericTypeManager.hpp"
#include "../../ast/nodes/statements/DeclarationNode.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/UndefinedException.hpp"
#include "../../errors/EnvironmentException.hpp"
#include "../../value/LambdaValue.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"
#include "../../constants/LambdaConstants.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>

using namespace errors;
using namespace runtimeTypes::global;

namespace evaluator {
namespace statements {

    Value DeclarationHandler::evaluateDeclaration(DeclarationNode* node)
    {
        validateVariableDeclaration(node);

        auto env = context->getEnvironment();

        // Evaluate initial value if present, otherwise use default
        Value initialValue = std::monostate{};
        if (node->getInitializer() && exprEvaluator)
        {
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

    Value DeclarationHandler::evaluateAssignment(AssignmentNode* node)
    {
        if (!exprEvaluator)
        {
            // This can happen during initialization phase before dependencies are set up
            // Log the error but don't throw to avoid breaking the initialization process
            std::cerr << "Warning: Expression evaluator not available for assignment evaluation" << std::endl;
            return std::monostate{};
        }

        // Evaluate the new value
        Value newValue = exprEvaluator->evaluate(node->getValue());

        // Check for lambda-to-interface conversion
        if (std::holds_alternative<std::shared_ptr<value::LambdaValue>>(newValue) &&
            node->getVariableType() == ValueType::OBJECT &&
            !node->getClassName().empty())
        {
            // Try to convert lambda to interface implementation
            newValue = convertLambdaToInterface(newValue, node->getClassName(), node->getLocation());
        }

        auto env = context->getEnvironment();

        // PRIORITY: Check for implicit field assignment first when in a constructor/instance context
        // This ensures field assignments work correctly even when there are shadowing parameters
        auto currentInstance = context->getCurrentInstance();
        if (currentInstance && node->getVariableType() == ValueType::VOID)
        {
            auto field = currentInstance->getField(node->getVariableName());
            if (field)
            {
                // This is an implicit field assignment - prioritize over variable shadowing
                currentInstance->setField(node->getVariableName(), newValue);
                return newValue;
            }
        }

        auto varDef = env->findVariable(node->getVariableName());

        if (!varDef)
        {
            // Check if this is a variable declaration (has type != VOID)
            if (node->getVariableType() != ValueType::VOID)
            {
                // This is a declaration - create the variable
                validateAssignmentAsDeclaration(node);

                // Validate that the class exists for object types (but skip native arrays)
                if (node->getVariableType() == ValueType::OBJECT && !node->getClassName().empty())
                {
                    std::string className = node->getClassName();
                    // Skip validation for native array types (e.g., "int[]", "string[]", "T[]")
                    if (className.find("[]") == std::string::npos)
                    {
                        // Skip validation for unresolved generic type parameters
                        // If the class name contains type parameters like T, K, V, skip validation
                        // since these should be resolved at instantiation time
                        bool hasUnresolvedTypeParams = false;

                        // Check for generic type parameter names
                        // Handle single letter params (T, K, V, E, etc.) or longer names (Element, Type, etc.)
                        auto isGenericTypeParameter = [](const std::string& name)
                        {
                            // Single uppercase letter is definitely a generic type parameter
                            if (name.length() == 1 && std::isupper(name[0]))
                            {
                                return true;
                            }
                            // Common generic type parameter patterns
                            if (name == "Element" || name == "Type" || name == "Key" || name == "Value" ||
                                name == "Item" || name == "Data" || name == "Node" || name == "Entry")
                            {
                                return true;
                            }
                            // Uppercase name that's likely a generic parameter (heuristic)
                            if (!name.empty() && std::isupper(name[0]) && name.length() <= 10)
                            {
                                // Additional heuristic: if it's all letters and starts with uppercase
                                bool allLetters = std::all_of(name.begin(), name.end(), ::isalpha);
                                if (allLetters)
                                {
                                    return true;
                                }
                            }
                            return false;
                        };

                        if (isGenericTypeParameter(className))
                        {
                            hasUnresolvedTypeParams = true;
                        }
                        // Handle array types containing generic parameters (T[], Element[], etc.)
                        else if (className.find("[]") != std::string::npos)
                        {
                            std::string baseType = className.substr(0, className.find("[]"));
                            if (isGenericTypeParameter(baseType))
                            {
                                hasUnresolvedTypeParams = true;
                            }
                        }
                        // Check for generic types with angle brackets
                        else if (className.find('<') != std::string::npos)
                        {
                            // Check for common generic type parameter names
                            if (className.find("<T>") != std::string::npos ||
                                className.find("<T,") != std::string::npos ||
                                className.find(",T>") != std::string::npos ||
                                className.find(",T,") != std::string::npos ||
                                className.find("<K,") != std::string::npos ||
                                className.find(",V>") != std::string::npos ||
                                className.find("<K>") != std::string::npos ||
                                className.find("<V>") != std::string::npos)
                            {
                                hasUnresolvedTypeParams = true;
                            }
                            // Check if this is a concrete generic instantiation (e.g., HashMap<String, Int>)
                            else if (utils::GenericTypeManager::isGenericInstantiation(className))
                            {
                                // For concrete instantiations, validate the base class exists
                                auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(className);
                                if (env->findClass(baseName))
                                {
                                    // Base class exists, this is a valid generic instantiation
                                    hasUnresolvedTypeParams = true; // Skip normal validation
                                }
                            }
                        }

                        if (!hasUnresolvedTypeParams)
                        {
                            validateClassExists(className, node->getLocation());
                        }
                    }
                }

                // Validate type compatibility for new variable declarations
                if (node->getVariableType() == ValueType::OBJECT && !node->getClassName().empty())
                {
                    // Resolve generic type parameters if present
                    std::string resolvedClassName = node->getClassName();
                    if (objEvaluator)
                    {
                        resolvedClassName = objEvaluator->resolveTypeParameterFromContext(node->getClassName());
                    }
                    validateTypeAssignment(node->getVariableType(), newValue, node->getVariableName(),
                                           node->getLocation(), resolvedClassName);
                }
                else
                {
                    validateTypeAssignment(node->getVariableType(), newValue, node->getVariableName(),
                                           node->getLocation());
                }

                // Create variable definition with class name for object types
                auto newVarDef = std::make_shared<VariableDefinition>(
                    node->getVariableName(),
                    node->getVariableType(),
                    newValue,
                    node->getIsFinal(),
                    (node->getVariableType() == ValueType::OBJECT && !node->getClassName().empty())
                        ? node->getClassName()
                        : ""
                );

                env->declareVariable(node->getVariableName(), newVarDef);
                return newValue;
            }
            else
            {
                // Check if this is a qualified static field assignment (contains ::)
                std::string varName = node->getVariableName();
                if (varName.find("::") != std::string::npos)
                {
                    // Parse the qualified name into parts
                    std::vector<std::string> parts;
                    size_t start = 0;
                    size_t pos = varName.find("::");

                    while (pos != std::string::npos)
                    {
                        parts.push_back(varName.substr(start, pos - start));
                        start = pos + 2; // Skip "::"
                        pos = varName.find("::", start);
                    }
                    parts.push_back(varName.substr(start));

                    // Handle qualified static field assignment: ClassName::fieldName
                    if (parts.size() == 2)
                    {
                        std::string className = parts[0];
                        std::string fieldName = parts[1];

                        auto classDef = env->findClass(className);
                        if (classDef)
                        {
                            auto field = classDef->getField(fieldName);
                            if (field && field->isStatic())
                            {
                                // This is a qualified static field assignment
                                field->setValue(newValue);
                                return newValue;
                            }
                        }

                        throw UndefinedException("Static field '" + varName + "' is not defined",
                                                 node->getLocation());
                    }
                    else
                    {
                        throw UndefinedException(
                            "Complex qualified variable assignment not supported: '" + varName + "'",
                            node->getLocation());
                    }
                }

                // Check if this might be a static field assignment using current class context
                auto classRegistry = env->getClassRegistry();

                // Check if we have a current class name stored (from method execution)
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
                            auto field = classDef->getField(node->getVariableName());
                            if (field && field->isStatic())
                            {
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
        if (node->getVariableType() != ValueType::VOID)
        {
            // Check if variable exists in current scope - this is always an error
            if (env->getScopeManager()->hasVariableInCurrentScope(node->getVariableName()))
            {
                throw EnvironmentException("Variable '" + node->getVariableName() +
                                           "' is already defined in this scope", node->getLocation());
            }

            // Check if we're trying to redeclare a function parameter within the same function
            if (varDef && env->getScopeManager()->isInFunction())
            {
                // We're in a function and the variable exists - check if it's a function parameter
                // by checking if it exists in the immediate parent function scope
                auto currentScope = env->getScopeManager()->getCurrentScope();
                if (currentScope && currentScope->getParent() &&
                    currentScope->getParent()->getType() == ScopeType::FUNCTION)
                {
                    // Check if the variable is defined in the function scope (i.e., it's a parameter)
                    if (currentScope->getParent()->hasVariableInCurrentScope(node->getVariableName()))
                    {
                        throw EnvironmentException("Variable '" + node->getVariableName() +
                                                   "' is already defined in this scope", node->getLocation());
                    }
                }

                // If we reach here, it's valid scope shadowing (e.g., global -> method)
                // Create new variable definition with the specified type
                ValueType declaredType = node->getVariableType();

                // Validate type compatibility
                if (declaredType == ValueType::OBJECT && !node->getClassName().empty())
                {
                    // Resolve generic type parameters if present
                    std::string resolvedClassName = node->getClassName();
                    if (objEvaluator)
                    {
                        resolvedClassName = objEvaluator->resolveTypeParameterFromContext(node->getClassName());
                    }
                    validateTypeAssignment(declaredType, newValue, node->getVariableName(),
                                           node->getLocation(), resolvedClassName);
                }
                else
                {
                    validateTypeAssignment(declaredType, newValue, node->getVariableName(), node->getLocation());
                }

                // Create new variable definition with class name for object types
                auto newVarDef = std::make_shared<VariableDefinition>(
                    node->getVariableName(),
                    declaredType,
                    newValue,
                    node->getIsFinal(),
                    (declaredType == ValueType::OBJECT && !node->getClassName().empty())
                        ? node->getClassName()
                        : ""
                );

                env->declareVariable(node->getVariableName(), newVarDef);
                return newValue;
            }

            // Variable exists but we're not in a function, or variable doesn't exist in parent scope
            if (varDef)
            {
                // This is valid scope shadowing (e.g., global -> method)
                // Create new variable definition with the specified type
                ValueType declaredType = node->getVariableType();

                // Validate type compatibility
                if (declaredType == ValueType::OBJECT && !node->getClassName().empty())
                {
                    // Resolve generic type parameters if present
                    std::string resolvedClassName = node->getClassName();
                    if (objEvaluator)
                    {
                        resolvedClassName = objEvaluator->resolveTypeParameterFromContext(node->getClassName());
                    }
                    validateTypeAssignment(declaredType, newValue, node->getVariableName(),
                                           node->getLocation(), resolvedClassName);
                }
                else
                {
                    validateTypeAssignment(declaredType, newValue, node->getVariableName(), node->getLocation());
                }

                // Create new variable definition with class name for object types
                auto newVarDef = std::make_shared<VariableDefinition>(
                    node->getVariableName(),
                    declaredType,
                    newValue,
                    node->getIsFinal(),
                    (declaredType == ValueType::OBJECT && !node->getClassName().empty())
                        ? node->getClassName()
                        : ""
                );

                env->declareVariable(node->getVariableName(), newVarDef);
                return newValue;
            }
        }

        // Existing variable assignment
        if (varDef->isFinal())
        {
            throw TypeException("Cannot modify final variable '" + node->getVariableName() + "'",
                                node->getLocation());
        }

        // Validate type compatibility for existing variable assignments
        if (varDef->getType() == ValueType::OBJECT && !varDef->getClassName().empty())
        {
            // Resolve generic type parameters if present
            std::string resolvedClassName = varDef->getClassName();
            if (objEvaluator)
            {
                resolvedClassName = objEvaluator->resolveTypeParameterFromContext(varDef->getClassName());
            }
            validateTypeAssignment(varDef->getType(), newValue, node->getVariableName(),
                                   node->getLocation(), resolvedClassName);
        }
        else
        {
            validateTypeAssignment(varDef->getType(), newValue, node->getVariableName(), node->getLocation());
        }

        varDef->setValue(newValue);
        return newValue;
    }

    // Helper methods

    void DeclarationHandler::validateVariableDeclaration(DeclarationNode* node)
    {
        auto env = context->getEnvironment();

        // Check if variable already exists in current scope
        if (env->getScopeManager()->hasVariableInCurrentScope(node->getVariableName()))
        {
            throw EnvironmentException("Variable '" + node->getVariableName() +
                                       "' is already defined in this scope", node->getLocation());
        }
    }

    void DeclarationHandler::validateAssignmentAsDeclaration(AssignmentNode* node)
    {
        auto env = context->getEnvironment();

        // Check if variable already exists in current scope
        if (env->getScopeManager()->hasVariableInCurrentScope(node->getVariableName()))
        {
            throw EnvironmentException("Variable '" + node->getVariableName() +
                                       "' is already defined in this scope", node->getLocation());
        }
    }

    void DeclarationHandler::validateClassExists(const std::string& className, const SourceLocation& location)
    {
        // Delegate to TypeValidator utility
        validation::TypeValidator::validateClassExists(className, location, context);
    }

    void DeclarationHandler::validateTypeAssignment(ValueType expectedType, const Value& value,
                                                    const std::string& variableName,
                                                    const SourceLocation& location)
    {
        // Delegate to TypeValidator utility
        validation::TypeValidator::validateAssignment(expectedType, value, variableName, location);
    }

    void DeclarationHandler::validateTypeAssignment(ValueType expectedType, const Value& value,
                                                    const std::string& variableName,
                                                    const SourceLocation& location,
                                                    const std::string& expectedClassName)
    {
        // Delegate to TypeValidator utility, passing context
        validation::TypeValidator::validateAssignment(expectedType, value, variableName, location, expectedClassName, context);
    }

    Value DeclarationHandler::convertLambdaToInterface(const Value& lambdaValue, const std::string& interfaceName,
                                                       const SourceLocation& location)
    {
        // Extract the lambda value
        auto lambdaPtr = std::get<std::shared_ptr<value::LambdaValue>>(lambdaValue);
        auto* lambdaNode = lambdaPtr->getLambda();

        // Get the interface definition from the environment
        auto env = context->getEnvironment();
        auto interfaceDef = env->findInterface(interfaceName);

        if (!interfaceDef)
        {
            return lambdaValue;
        }

        // Check if the interface is functional (has exactly one method)
        if (!interfaceDef->isFunctionalInterface())
        {
            auto methodSignatures = interfaceDef->getMethodSignatures();
            throw TypeException(
                "Cannot assign lambda to non-functional interface '" + interfaceName + "'. " +
                "Lambdas can only be assigned to interfaces with exactly one method. " +
                "Interface '" + interfaceName + "' has " + std::to_string(methodSignatures.size()) + " methods. " +
                "Consider using a functional interface (single method) or implement the interface explicitly.",
                location
            );
        }

        // Create the lambda implementation class
        auto implClass = interfaceDef->createLambdaImplementation(lambdaNode);
        if (!implClass)
        {
            return lambdaValue;
        }

        // Create an instance of the implementation class
        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(implClass);

        // Store the lambda in a special field that the implementation can access
        instance->setField(constants::lambda::LAMBDA_FIELD_NAME, lambdaValue);

        return instance;
    }

} // namespace statements
} // namespace evaluator
