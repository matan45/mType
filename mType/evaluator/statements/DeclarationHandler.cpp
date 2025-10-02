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
#include <iostream>

using namespace errors;
using namespace runtimeTypes::global;

namespace evaluator
{
    namespace statements
    {
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

        // Helper method implementations

        Value DeclarationHandler::handleLambdaConversion(Value value, AssignmentNode* node)
        {
            if (std::holds_alternative<std::shared_ptr<value::LambdaValue>>(value) &&
                node->getVariableType() == ValueType::OBJECT &&
                !node->getClassName().empty())
            {
                return convertLambdaToInterface(value, node->getClassName(), node->getLocation());
            }
            return value;
        }

        bool DeclarationHandler::tryImplicitFieldAssignment(const Value& newValue, AssignmentNode* node)
        {
            auto currentInstance = context->getCurrentInstance();
            if (currentInstance && node->getVariableType() == ValueType::VOID)
            {
                auto field = currentInstance->getField(node->getVariableName());
                if (field)
                {
                    currentInstance->setField(node->getVariableName(), newValue);
                    return true;
                }
            }
            return false;
        }

        std::shared_ptr<runtimeTypes::global::VariableDefinition> DeclarationHandler::createVariableDefinition(
            AssignmentNode* node, const Value& value)
        {
            return std::make_shared<runtimeTypes::global::VariableDefinition>(
                node->getVariableName(),
                node->getVariableType(),
                value,
                node->getIsFinal(),
                (node->getVariableType() == ValueType::OBJECT && !node->getClassName().empty())
                    ? node->getClassName()
                    : ""
            );
        }

        Value DeclarationHandler::handleNewVariableDeclaration(const Value& newValue, AssignmentNode* node)
        {
            auto env = context->getEnvironment();

            // Validate assignment as declaration
            validateAssignmentAsDeclaration(node);

            // Validate that the class exists for object types (but skip native arrays and generics)
            if (node->getVariableType() == ValueType::OBJECT && !node->getClassName().empty())
            {
                std::string className = node->getClassName();

                // Skip validation for native array types and unresolved generic type parameters
                bool shouldValidate = className.find("[]") == std::string::npos &&
                    !utils::GenericTypeManager::hasUnresolvedGenericParams(className);

                // Additional check for concrete generic instantiation with valid base class
                if (shouldValidate && utils::GenericTypeManager::isGenericInstantiation(className))
                {
                    auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(className);
                    if (env->findClass(baseName))
                    {
                        shouldValidate = false; // Valid generic instantiation, skip validation
                    }
                }

                if (shouldValidate)
                {
                    validateClassExists(className, node->getLocation());
                }
            }

            // Validate type compatibility
            if (node->getVariableType() == ValueType::OBJECT && !node->getClassName().empty())
            {
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

            // Create and declare the variable
            auto newVarDef = createVariableDefinition(node, newValue);
            env->declareVariable(node->getVariableName(), newVarDef);
            return newValue;
        }

        Value DeclarationHandler::handleQualifiedStaticAssignment(const Value& newValue, AssignmentNode* node)
        {
            auto env = context->getEnvironment();
            std::string varName = node->getVariableName();

            // Parse the qualified name into parts
            std::vector<std::string> parts;
            size_t start = 0;
            size_t pos = varName.find("::");

            while (pos != std::string::npos)
            {
                parts.push_back(varName.substr(start, pos - start));
                start = pos + 2;
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
                        field->setValue(newValue);
                        return newValue;
                    }
                }

                throw UndefinedException("Static field '" + varName + "' is not defined",
                                         node->getLocation());
            }

            throw UndefinedException(
                "Complex qualified variable assignment not supported: '" + varName + "'",
                node->getLocation());
        }

        Value DeclarationHandler::handleUnqualifiedStaticAssignment(const Value& newValue, AssignmentNode* node)
        {
            auto env = context->getEnvironment();

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
                            field->setValue(newValue);
                            return newValue;
                        }
                    }
                }
            }

            // Variable not found
            throw UndefinedException("Variable '" + node->getVariableName() + "' is not defined",
                                     node->getLocation());
        }

        Value DeclarationHandler::handleScopeShadowing(const Value& newValue, AssignmentNode* node,
                                                       std::shared_ptr<runtimeTypes::global::VariableDefinition> varDef)
        {
            auto env = context->getEnvironment();

            // Check if variable exists in current scope - this is always an error
            if (env->getScopeManager()->hasVariableInCurrentScope(node->getVariableName()))
            {
                throw EnvironmentException("Variable '" + node->getVariableName() +
                                           "' is already defined in this scope", node->getLocation());
            }

            // Check if we're trying to redeclare a function parameter within the same function
            if (varDef && env->getScopeManager()->isInFunction())
            {
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
            }

            // Valid scope shadowing - create new variable
            ValueType declaredType = node->getVariableType();

            // Validate type compatibility
            if (declaredType == ValueType::OBJECT && !node->getClassName().empty())
            {
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

            // Create new variable definition
            auto newVarDef = createVariableDefinition(node, newValue);
            env->declareVariable(node->getVariableName(), newVarDef);
            return newValue;
        }

        Value DeclarationHandler::handleExistingVariableAssignment(const Value& newValue, AssignmentNode* node,
                                                                   std::shared_ptr<
                                                                       runtimeTypes::global::VariableDefinition> varDef)
        {
            // Check if variable is final
            if (varDef->isFinal())
            {
                throw TypeException("Cannot modify final variable '" + node->getVariableName() + "'",
                                    node->getLocation());
            }

            // Validate type compatibility
            if (varDef->getType() == ValueType::OBJECT && !varDef->getClassName().empty())
            {
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

        Value DeclarationHandler::evaluateAssignment(AssignmentNode* node)
        {
            if (!exprEvaluator)
            {
                std::cerr << "Warning: Expression evaluator not available for assignment evaluation" << std::endl;
                return std::monostate{};
            }

            // Evaluate and convert value if needed
            Value newValue = exprEvaluator->evaluate(node->getValue());
            newValue = handleLambdaConversion(newValue, node);

            // Try implicit field assignment first (priority for constructor/instance context)
            if (tryImplicitFieldAssignment(newValue, node))
            {
                return newValue;
            }

            auto env = context->getEnvironment();
            auto varDef = env->findVariable(node->getVariableName());

            // Variable not found
            if (!varDef)
            {
                // Is this a new variable declaration?
                if (node->getVariableType() != ValueType::VOID)
                {
                    return handleNewVariableDeclaration(newValue, node);
                }

                // Check for qualified static field (ClassName::field)
                if (node->getVariableName().find("::") != std::string::npos)
                {
                    return handleQualifiedStaticAssignment(newValue, node);
                }

                // Check for unqualified static field (using current class context)
                return handleUnqualifiedStaticAssignment(newValue, node);
            }

            // Variable exists - check if we're trying to redeclare it
            if (node->getVariableType() != ValueType::VOID)
            {
                return handleScopeShadowing(newValue, node, varDef);
            }

            // Simple assignment to existing variable
            return handleExistingVariableAssignment(newValue, node, varDef);
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
            validation::TypeValidator::validateAssignment(expectedType, value, variableName, location,
                                                          expectedClassName, context);
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
    } 
} 
