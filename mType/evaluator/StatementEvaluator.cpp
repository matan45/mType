#include "StatementEvaluator.hpp"
#include "../services/ImportManager.hpp"
#include <filesystem>
#include <iostream>
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../ast/nodes/statements/DeclarationNode.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/statements/IfNode.hpp"
#include "../ast/nodes/statements/WhileNode.hpp"
#include "../ast/nodes/statements/DoWhileNode.hpp"
#include "../ast/nodes/statements/ForNode.hpp"
#include "../ast/nodes/statements/SwitchNode.hpp"
#include "../ast/nodes/statements/CaseNode.hpp"
#include "../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../ast/nodes/statements/ImportNode.hpp"
#include "../ast/nodes/statements/NativeFunctionNode.hpp"
#include "../ast/nodes/functions/FunctionNode.hpp"
#include "../ast/nodes/functions/ReturnNode.hpp"
#include "../environment/manager/Scope.hpp"
#include "../exception/BreakException.hpp"
#include "../exception/ContinueException.hpp"
#include "../exception/ReturnException.hpp"
#include "../errors/TypeException.hpp"
#include "../errors/UndefinedException.hpp"
#include "../errors/RuntimeException.hpp"
#include "../errors/ScriptException.hpp"
#include "../errors/EnvironmentException.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include "../ast/nodes/statements/ContinueNode.hpp"
#include "../ast/nodes/statements/ForEachNode.hpp"
#include "utils/ScopeGuard.hpp"
#include "ExpressionEvaluator.hpp"
#include "ObjectEvaluator.hpp"
#include "../value/NativeArray.hpp"
#include "../value/LambdaValue.hpp"
#include "../runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"

namespace evaluator
{
    using namespace errors;
    using namespace exception;
    using namespace runtimeTypes::global;
    using namespace environment::manager;
    using namespace services;
    using namespace utils;

    StatementEvaluator::StatementEvaluator(std::shared_ptr<EvaluationContext> ctx)
        : context(ctx), flowManager(std::make_unique<ControlFlowManager>()),
          exprEvaluator(nullptr), objEvaluator(nullptr)
    {
    }

    Value StatementEvaluator::evaluate(ASTNode* node)
    {
        if (!node)
        {
            return std::monostate{};
        }

        // Try to handle with this evaluator first
        if (!canHandle(node))
        {
            // Delegate to other evaluators if this one can't handle it
            if (exprEvaluator && exprEvaluator->canHandle(node))
            {
                return exprEvaluator->evaluate(node);
            }
            else if (objEvaluator && objEvaluator->canHandle(node))
            {
                return objEvaluator->evaluate(node);
            }
            return std::monostate{};
        }

        // Dispatch to appropriate evaluation method based on node type
        if (auto progNode = dynamic_cast<ProgramNode*>(node))
        {
            return evaluateProgramNode(progNode);
        }
        if (auto blockNode = dynamic_cast<BlockNode*>(node))
        {
            return evaluateBlockNode(blockNode);
        }
        if (auto declNode = dynamic_cast<DeclarationNode*>(node))
        {
            return evaluateDeclarationNode(declNode);
        }
        if (auto assignNode = dynamic_cast<AssignmentNode*>(node))
        {
            return evaluateAssignmentNode(assignNode);
        }
        if (auto ifNode = dynamic_cast<IfNode*>(node))
        {
            return evaluateIfNode(ifNode);
        }
        if (auto whileNode = dynamic_cast<WhileNode*>(node))
        {
            return evaluateWhileNode(whileNode);
        }
        if (auto doWhileNode = dynamic_cast<DoWhileNode*>(node))
        {
            return evaluateDoWhileNode(doWhileNode);
        }
        if (auto forNode = dynamic_cast<ForNode*>(node))
        {
            return evaluateForNode(forNode);
        }
        if (auto forEachNode = dynamic_cast<ForEachNode*>(node))
        {
            return evaluateForEachNode(forEachNode);
        }
        if (auto breakNode = dynamic_cast<BreakNode*>(node))
        {
            return evaluateBreakNode(breakNode);
        }
        if (auto continueNode = dynamic_cast<ContinueNode*>(node))
        {
            return evaluateContinueNode(continueNode);
        }
        if (auto switchNode = dynamic_cast<SwitchNode*>(node))
        {
            return evaluateSwitchNode(switchNode);
        }
        if (auto caseNode = dynamic_cast<CaseNode*>(node))
        {
            return evaluateCaseNode(caseNode);
        }
        if (auto defaultNode = dynamic_cast<DefaultCaseNode*>(node))
        {
            return evaluateDefaultCaseNode(defaultNode);
        }
        if (auto importNode = dynamic_cast<ImportNode*>(node))
        {
            return evaluateImportNode(importNode);
        }
        if (auto funcNode = dynamic_cast<FunctionNode*>(node))
        {
            return evaluateFunctionNode(funcNode);
        }
        if (auto retNode = dynamic_cast<ReturnNode*>(node))
        {
            return evaluateReturnNode(retNode);
        }
        if (auto nativeNode = dynamic_cast<NativeFunctionNode*>(node))
        {
            return evaluateNativeFunctionNode(nativeNode);
        }

        return std::monostate{};
    }

    bool StatementEvaluator::canHandle(ASTNode* node) const
    {
        return isStatementNode(node);
    }

    bool StatementEvaluator::shouldBreakOrContinue() const
    {
        return flowManager->shouldBreakOrContinue();
    }

    void StatementEvaluator::handleBreak()
    {
        flowManager->handleBreak();
    }

    void StatementEvaluator::handleContinue()
    {
        flowManager->handleContinue();
    }

    void StatementEvaluator::enterLoop()
    {
        flowManager->enterLoop();
    }

    void StatementEvaluator::exitLoop()
    {
        flowManager->exitLoop();
    }

    bool StatementEvaluator::isInLoop() const
    {
        return flowManager->isInLoop();
    }

    void StatementEvaluator::enterBreakableContext()
    {
        flowManager->enterBreakableContext();
    }

    void StatementEvaluator::exitBreakableContext()
    {
        flowManager->exitBreakableContext();
    }

    bool StatementEvaluator::isInBreakableContext() const
    {
        return flowManager->isInBreakableContext();
    }

    void StatementEvaluator::resetLoopFlags()
    {
        flowManager->resetLoopFlags();
    }

    void StatementEvaluator::setExpressionEvaluator(ExpressionEvaluator* evaluator)
    {
        exprEvaluator = evaluator;
    }

    void StatementEvaluator::setObjectEvaluator(ObjectEvaluator* evaluator)
    {
        objEvaluator = evaluator;
    }

    bool StatementEvaluator::isStatementNode(ASTNode* node) const
    {
        return dynamic_cast<ProgramNode*>(node) ||
            dynamic_cast<BlockNode*>(node) ||
            dynamic_cast<DeclarationNode*>(node) ||
            dynamic_cast<AssignmentNode*>(node) ||
            dynamic_cast<IfNode*>(node) ||
            dynamic_cast<WhileNode*>(node) ||
            dynamic_cast<DoWhileNode*>(node) ||
            dynamic_cast<ForNode*>(node) ||
            dynamic_cast<ForEachNode*>(node) ||
            dynamic_cast<BreakNode*>(node) ||
            dynamic_cast<ContinueNode*>(node) ||
            dynamic_cast<SwitchNode*>(node) ||
            dynamic_cast<CaseNode*>(node) ||
            dynamic_cast<DefaultCaseNode*>(node) ||
            dynamic_cast<ImportNode*>(node) ||
            dynamic_cast<FunctionNode*>(node) ||
            dynamic_cast<ReturnNode*>(node) ||
            dynamic_cast<NativeFunctionNode*>(node);
    }

    Value StatementEvaluator::executeStatementList(const std::vector<std::unique_ptr<ASTNode>>& statements)
    {
        Value lastValue = std::monostate{};

        for (size_t i = 0; i < statements.size(); i++)
        {
            const auto& statement = statements[i];
            // Delegate evaluation based on statement type
            if (canHandle(statement.get()))
            {
                lastValue = evaluate(statement.get());
            }
            else if (exprEvaluator && exprEvaluator->canHandle(statement.get()))
            {
                lastValue = exprEvaluator->evaluate(statement.get());
            }
            else if (objEvaluator && objEvaluator->canHandle(statement.get()))
            {
                lastValue = objEvaluator->evaluate(statement.get());
            }

            // Check for control flow changes
            if (context->shouldReturn() || shouldBreakOrContinue())
            {
                break;
            }
        }

        return lastValue;
    }

    Value StatementEvaluator::evaluateProgramNode(ProgramNode* node)
    {
        return executeStatementList(node->getStatements());
    }

    Value StatementEvaluator::evaluateBlockNode(BlockNode* node)
    {
        auto env = context->getEnvironment();
        env->enterScope("", ScopeType::BLOCK);

        Value lastValue = std::monostate{};

        try
        {
            lastValue = executeStatementList(node->getStatements());
        }
        catch (...)
        {
            env->exitScope();
            throw;
        }

        env->exitScope();
        return lastValue;
    }

    Value StatementEvaluator::evaluateDeclarationNode(DeclarationNode* node)
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

    void StatementEvaluator::validateVariableDeclaration(DeclarationNode* node)
    {
        auto env = context->getEnvironment();

        // Check if variable already exists in current scope
        if (env->getScopeManager()->hasVariableInCurrentScope(node->getVariableName()))
        {
            throw EnvironmentException("Variable '" + node->getVariableName() +
                                       "' is already defined in this scope", node->getLocation());
        }
    }

    void StatementEvaluator::validateTypeAssignment(ValueType expectedType, const Value& value,
                                                    const std::string& variableName,
                                                    const SourceLocation& location)
    {
        // Skip validation for void initializers (default values)
        if (std::holds_alternative<std::monostate>(value) && expectedType != ValueType::VOID)
        {
            return; // Allow default initialization
        }

        ValueType actualType = utils::ValueConverter::getValueType(value);

        // Allow null assignment to object types
        if (actualType == ValueType::NULL_TYPE && expectedType == ValueType::OBJECT)
        {
            return;
        }

        // Allow exact type matches
        if (actualType == expectedType)
        {
            return;
        }

        // Allow valid implicit conversions
        if (isValidTypeConversion(actualType, expectedType))
        {
            return;
        }

        // Type mismatch
        throw TypeException("Type mismatch for variable '" + variableName + "': expected " +
                            utils::ValueConverter::valueTypeToString(expectedType) +
                            " but got " + utils::ValueConverter::valueTypeToString(actualType),
                            location);
    }

    void StatementEvaluator::validateTypeAssignment(ValueType expectedType, const Value& value,
                                                    const std::string& variableName,
                                                    const SourceLocation& location,
                                                    const std::string& expectedClassName)
    {
        // Skip validation for void initializers (default values)
        if (std::holds_alternative<std::monostate>(value) && expectedType != ValueType::VOID)
        {
            return; // Allow default initialization
        }

        ValueType actualType = utils::ValueConverter::getValueType(value);

        // Allow null assignment to object types
        if (actualType == ValueType::NULL_TYPE && expectedType == ValueType::OBJECT)
        {
            return;
        }

        // Allow exact type matches
        if (actualType == expectedType)
        {
            // For object types, validate class compatibility
            if (actualType == ValueType::OBJECT && expectedType == ValueType::OBJECT && !expectedClassName.empty())
            {
                validateObjectTypeCompatibility(value, variableName, location, expectedClassName);
            }
            return;
        }

        // Allow valid implicit conversions
        if (isValidTypeConversion(actualType, expectedType))
        {
            return;
        }

        // Type mismatch
        throw TypeException("Type mismatch for variable '" + variableName + "': expected " +
                            utils::ValueConverter::valueTypeToString(expectedType) +
                            " but got " + utils::ValueConverter::valueTypeToString(actualType),
                            location);
    }

    void StatementEvaluator::validateAssignmentAsDeclaration(AssignmentNode* node)
    {
        auto env = context->getEnvironment();

        // Check if variable already exists in current scope
        if (env->getScopeManager()->hasVariableInCurrentScope(node->getVariableName()))
        {
            throw EnvironmentException("Variable '" + node->getVariableName() +
                                       "' is already defined in this scope", node->getLocation());
        }
    }

    void StatementEvaluator::validateClassExists(const std::string& className, const SourceLocation& location)
    {
        auto env = context->getEnvironment();

        // Try to resolve type parameters from current object context first
        std::string resolvedClassName = className;
        auto currentInstance = context->getCurrentInstance();
        if (currentInstance && className.find('<') != std::string::npos && className.find('T') != std::string::npos)
        {
            auto instanceClassDef = currentInstance->getClassDefinition();
            if (instanceClassDef)
            {
                std::string instanceClassName = instanceClassDef->getName(); // e.g., "Set<int>"

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

        // Check if the class or interface is defined
        if (!env->findClass(resolvedClassName) && !env->findInterface(resolvedClassName))
        {
            throw UndefinedException("Class '" + resolvedClassName + "' is not defined", location);
        }
    }

    bool StatementEvaluator::isValidTypeConversion(ValueType from, ValueType to)
    {
        // Allow int to float conversion (common implicit conversion)
        if (from == ValueType::INT && to == ValueType::FLOAT)
        {
            return true;
        }

        // Add other valid conversions here if needed
        // For example, you might allow:
        // - bool to int (false = 0, true = 1)
        // - char to int
        // etc.

        return false; // No other conversions allowed for now
    }

    bool StatementEvaluator::isGenericTypeCompatible(const std::string& actualClassName,
                                                     const std::string& expectedClassName)
    {
        // Exact match is always compatible
        if (actualClassName == expectedClassName)
        {
            return true;
        }

        // Handle generic type compatibility
        // Extract base class name (e.g., "Set" from "Set<T>" or "Set<int>")
        auto extractBaseClassName = [](const std::string& className) -> std::string
        {
            size_t pos = className.find('<');
            if (pos != std::string::npos)
            {
                return className.substr(0, pos);
            }
            return className;
        };

        std::string actualBase = extractBaseClassName(actualClassName);
        std::string expectedBase = extractBaseClassName(expectedClassName);

        // If base class names match, consider compatible for generic types
        if (actualBase == expectedBase)
        {
            return true;
        }

        return false;
    }

    void StatementEvaluator::validateObjectTypeCompatibility(const Value& value,
                                                             const std::string& variableName,
                                                             const SourceLocation& location,
                                                             const std::string& expectedClassName)
    {
        // Handle Object instances
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(value))
        {
            auto objInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(value);
            if (objInstance)
            {
                std::string actualClassName = objInstance->getClassDefinition()->getName();

                // Check if the actual class matches the expected class or implements the expected interface
                if (!isGenericTypeCompatible(actualClassName, expectedClassName))
                {
                    // Check if expectedClassName is an interface that the actual class implements
                    auto classDefinition = objInstance->getClassDefinition();
                    if (!classDefinition->implementsInterface(expectedClassName))
                    {
                        throw TypeException("Object type mismatch for variable '" + variableName + "': expected " +
                                            expectedClassName + " but got " + actualClassName, location);
                    }
                }

                // TODO: In a full implementation, this would also check class hierarchies
                // to allow assignments like Derived -> Base (inheritance compatibility)
            }
            return;
        }

        // Handle NativeArray (1D arrays)
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(value))
        {
            // NativeArray represents 1D arrays, expected type should be like "int[]"
            if (expectedClassName.find("[]") != std::string::npos)
            {
                // It's an array type, allow the assignment
                return;
            }
            else
            {
                throw TypeException("Type mismatch for variable '" + variableName + "': expected " +
                                    expectedClassName + " but got array type", location);
            }
        }

        // Handle FlatMultiArray (multi-dimensional arrays)
        if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(value))
        {
            // FlatMultiArray represents N-dimensional arrays, expected type should be like "int[][]"
            if (expectedClassName.find("[]") != std::string::npos)
            {
                // It's an array type, allow the assignment
                return;
            }
            else
            {
                throw TypeException("Type mismatch for variable '" + variableName + "': expected " +
                                    expectedClassName + " but got multi-dimensional array type", location);
            }
        }

        // If we get here, the value type is not compatible with object expectations
        // This should not happen if called correctly, but add safety check
    }

    Value StatementEvaluator::evaluateAssignmentNode(AssignmentNode* node)
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
            !node->getClassName().empty()) {

            // Try to convert lambda to interface implementation
            newValue = convertLambdaToInterface(newValue, node->getClassName());
        }

        // Type detection is now working correctly

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
                                auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(
                                    className);
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
                        std::string originalClassName = node->getClassName();
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
                        std::string originalClassName = node->getClassName();
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
                        std::string originalClassName = node->getClassName();
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
                std::string originalClassName = varDef->getClassName();
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

    Value StatementEvaluator::evaluateIfNode(IfNode* node)
    {
        if (!exprEvaluator)
        {
            throw TypeException("Expression evaluator not available for if statement evaluation");
        }

        Value conditionValue = exprEvaluator->evaluate(node->getCondition());
        bool isTrue = exprEvaluator->isTruthy(conditionValue);

        if (isTrue)
        {
            return evaluate(node->getThenStatement());
        }
        else if (node->getElseStatement())
        {
            return evaluate(node->getElseStatement());
        }

        return std::monostate{};
    }

    Value StatementEvaluator::evaluateWhileNode(WhileNode* node)
    {
        if (!exprEvaluator)
        {
            throw TypeException("Expression evaluator not available for while loop evaluation");
        }

        Value lastValue = std::monostate{};
        resetLoopFlags();
        enterLoop(); // Mark that we're entering a loop

        try
        {
            while (true)
            {
                Value conditionValue = exprEvaluator->evaluate(node->getCondition());
                if (!exprEvaluator->isTruthy(conditionValue))
                {
                    break;
                }

                try
                {
                    lastValue = evaluate(node->getBody());
                }
                catch (const ContinueException&)
                {
                    // Continue caught - reset flags and continue to next iteration
                    resetLoopFlags();
                    continue; // Continue the while loop
                }

                if (context->shouldReturn() || flowManager->isBreaking())
                {
                    break;
                }

                if (flowManager->isContinuing())
                {
                    resetLoopFlags();
                    continue;
                }
            }
        }
        catch (const BreakException&)
        {
            // Break caught, exit loop normally
        }

        resetLoopFlags();
        exitLoop(); // Mark that we're exiting the loop
        return lastValue;
    }

    Value StatementEvaluator::evaluateDoWhileNode(DoWhileNode* node)
    {
        if (!exprEvaluator)
        {
            throw TypeException("Expression evaluator not available for do-while loop evaluation");
        }

        Value lastValue = std::monostate{};
        resetLoopFlags();
        enterLoop(); // Mark that we're entering a loop

        try
        {
            do
            {
                try
                {
                    lastValue = evaluate(node->getBody());
                }
                catch (const ContinueException&)
                {
                    // Continue caught - reset flags and continue to condition check
                    resetLoopFlags();
                    // Fall through to condition check
                }

                if (context->shouldReturn() || flowManager->isBreaking())
                {
                    break;
                }

                if (flowManager->isContinuing())
                {
                    resetLoopFlags();
                    // Check condition before next iteration
                }

                Value conditionValue = exprEvaluator->evaluate(node->getCondition());
                if (!exprEvaluator->isTruthy(conditionValue))
                {
                    break;
                }
            }
            while (true);
        }
        catch (const BreakException&)
        {
            // Break caught, exit loop normally
        }

        resetLoopFlags();
        exitLoop(); // Mark that we're exiting the loop
        return lastValue;
    }

    Value StatementEvaluator::evaluateForNode(ForNode* node)
    {
        if (!exprEvaluator)
        {
            throw TypeException("Expression evaluator not available for for loop evaluation");
        }

        auto env = context->getEnvironment();
        env->enterScope("", ScopeType::BLOCK);

        Value lastValue = std::monostate{};
        resetLoopFlags();
        enterLoop(); // Mark that we're entering a loop

        try
        {
            // Initialize
            if (node->getInitialization())
            {
                evaluate(node->getInitialization());
            }

            // Loop
            while (true)
            {
                // Check condition
                if (node->getCondition())
                {
                    Value conditionValue = exprEvaluator->evaluate(node->getCondition());
                    if (!exprEvaluator->isTruthy(conditionValue))
                    {
                        break;
                    }
                }

                // Execute body
                try
                {
                    lastValue = evaluate(node->getBody());
                }
                catch (const ContinueException&)
                {
                    // Continue caught - reset flags and continue to update/next iteration
                    resetLoopFlags();
                    // Fall through to update
                }

                if (context->shouldReturn() || flowManager->isBreaking())
                {
                    break;
                }

                if (flowManager->isContinuing())
                {
                    resetLoopFlags();
                }

                // Update
                if (node->getUpdate())
                {
                    evaluate(node->getUpdate());
                }
            }
        }
        catch (const BreakException&)
        {
            // Break caught, exit loop normally
        }
        catch (...)
        {
            env->exitScope();
            exitLoop(); // Clean up loop state even on exception
            throw;
        }

        env->exitScope();
        resetLoopFlags();
        exitLoop(); // Mark that we're exiting the loop
        return lastValue;
    }

    Value StatementEvaluator::evaluateBreakNode(BreakNode* node)
    {
        if (!isInBreakableContext())
        {
            throw errors::RuntimeException("'break' statement must be inside a loop or switch statement",
                                           node->getLocation());
        }
        handleBreak();
        throw BreakException();
    }

    Value StatementEvaluator::evaluateContinueNode(ContinueNode* node)
    {
        if (!isInLoop())
        {
            throw errors::RuntimeException("'continue' statement must be inside a loop", node->getLocation());
        }
        handleContinue();
        throw ContinueException();
    }

    Value StatementEvaluator::evaluateReturnNode(ReturnNode* node)
    {
        Value returnValue = std::monostate{};

        if (node->getReturnValue() && exprEvaluator)
        {
            returnValue = exprEvaluator->evaluate(node->getReturnValue());
        }


        context->pushReturnValue(returnValue);
        context->setReturned(true);

        throw ReturnException(returnValue);
    }

    // Switch statement implementation
    Value StatementEvaluator::evaluateSwitchNode(SwitchNode* node)
    {
        if (!exprEvaluator)
        {
            throw TypeException("Expression evaluator not available for switch statement evaluation");
        }

        // Evaluate the switch expression
        Value switchValue = exprEvaluator->evaluate(node->getExpression());

        Value lastValue = std::monostate{};
        bool foundMatch = false;
        bool executeRemaining = false; // For fallthrough behavior

        resetLoopFlags(); // Reset any existing break/continue flags

        // Enter breakable context for switch statement
        enterBreakableContext();

        try
        {
            // Iterate through all cases
            for (const auto& casePtr : node->getCases())
            {
                if (auto caseNode = dynamic_cast<CaseNode*>(casePtr.get()))
                {
                    // Regular case
                    if (!foundMatch && !executeRemaining)
                    {
                        // Check if case value matches switch value
                        Value caseValue = exprEvaluator->evaluate(caseNode->getValue());
                        if (ValueConverter::compareValues(switchValue, caseValue))
                        {
                            foundMatch = true;
                            executeRemaining = true;
                        }
                    }

                    // Execute case statements if we're in fallthrough mode
                    if (executeRemaining)
                    {
                        for (const auto& statement : caseNode->getStatements())
                        {
                            try
                            {
                                lastValue = evaluate(statement.get());
                            }
                            catch (const BreakException&)
                            {
                                // Break encountered - exit switch
                                resetLoopFlags();
                                exitBreakableContext();
                                return lastValue;
                            }

                            // Check for other control flow interruptions
                            if (context->shouldReturn())
                            {
                                exitBreakableContext();
                                return lastValue;
                            }
                        }
                    }
                }
                else if (auto defaultNode = dynamic_cast<DefaultCaseNode*>(casePtr.get()))
                {
                    // Default case - execute if no match found or if we're in fallthrough
                    if (!foundMatch || executeRemaining)
                    {
                        foundMatch = true; // Mark as found for fallthrough logic
                        executeRemaining = true;

                        for (const auto& statement : defaultNode->getStatements())
                        {
                            try
                            {
                                lastValue = evaluate(statement.get());
                            }
                            catch (const BreakException&)
                            {
                                // Break encountered - exit switch
                                resetLoopFlags();
                                exitBreakableContext();
                                return lastValue;
                            }

                            // Check for other control flow interruptions
                            if (context->shouldReturn())
                            {
                                exitBreakableContext();
                                return lastValue;
                            }
                        }

                        // Default case has implicit break - exit switch after executing default
                        exitBreakableContext();
                        return lastValue;
                    }
                }
            }
        }
        catch (const BreakException&)
        {
            // Break caught at switch level
            resetLoopFlags();
        }

        // Exit breakable context before returning
        exitBreakableContext();
        return lastValue;
    }

    Value StatementEvaluator::evaluateCaseNode(CaseNode* node)
    {
        // Case nodes should not be evaluated directly - they are handled within switch statements
        throw TypeException("Case nodes should not be evaluated directly outside of switch statement");
    }

    Value StatementEvaluator::evaluateDefaultCaseNode(DefaultCaseNode* node)
    {
        // Default case nodes should not be evaluated directly - they are handled within switch statements
        throw TypeException("Default case nodes should not be evaluated directly outside of switch statement");
    }

    Value StatementEvaluator::evaluateImportNode(ImportNode* node)
    {
        // Check if import is already resolved (e.g., from JSON deserialization)
        if (node->isResolved() && node->getImportedAST())
        {
            // Directly evaluate the embedded AST instead of resolving from files
            ASTNode* importedAST = node->getImportedAST();

            // Evaluate the embedded AST directly using the current evaluation context
            // This should register all classes, functions, etc. from the import
            if (importedAST)
            {
                // Recursively evaluate the imported AST using the statement evaluator
                Value result = this->evaluate(importedAST);
            }

            return std::monostate{}; // Import completed successfully
        }

        // Fall back to normal ImportManager-based resolution for non-resolved imports
        auto env = context->getEnvironment();
        auto importManager = env->getImportManager();

        if (!importManager)
        {
            throw TypeException("Import manager not available for import evaluation");
        }

        std::string filePath = node->getFilePath();

        // Determine execution mode based on file extension
        //bool isCacheMode = filePath.ends_with(".mtc");

        // For cache mode with release directory, keep .mtc paths for both tracking and resolution
        // For normal mode, use .mt paths for tracking consistency
        //std::string trackingPath = filePath;
        //std::string resolutionPath = filePath;

        /* if (!isCacheMode && filePath.ends_with(".mtc")) {
             trackingPath = filePath.substr(0, filePath.length() - 1); // Remove 'c' to get .mt
             resolutionPath = trackingPath;
         } else if (isCacheMode && !filePath.ends_with(".mtc")) {
             // Convert .mt to .mtc for cache mode
             resolutionPath = filePath + "c";
             trackingPath = resolutionPath;
         }*/

        // Use appropriate path for resolution based on execution mode
        std::string resolvedPath = importManager->resolvePath(filePath);

        // Check if already evaluated to avoid re-evaluation
        if (importManager->isEvaluated(resolvedPath))
        {
            return std::monostate{}; // Already imported
        }

        // Check for circular imports
        if (importManager->isBeingEvaluated(resolvedPath))
        {
            throw TypeException("Circular import detected: " + filePath + " is already being imported");
        }

        // Mark as being evaluated to prevent circular imports
        importManager->markAsBeingEvaluated(resolvedPath);

        try
        {
            /* if (isCacheMode) {
                 // Cache mode: Load pre-compiled .mtc file
                 // First try the release directory
                 std::string releaseDir = "release";
                 std::filesystem::path filePath_obj(filePath);
                 std::string filename = filePath_obj.filename().string();
                 std::string releasePath = releaseDir + "/" + filename;
 
                 if (std::filesystem::exists(releasePath)) {
                     importedAST = importManager->loadFromMtcFile(releasePath);
                 } else {
                     // Fallback to original path resolution
                     std::string resolvedMtcPath = importManager->resolvePath(filePath);
                     importedAST = importManager->loadFromMtcFile(resolvedMtcPath);
                 }
 
                 if (!importedAST) {
                     throw TypeException("Failed to load cached imported file: " + filePath);
                 }
             } else {
                 // Normal mode: Parse .mt file
                 importedAST = importManager->parseAndCacheAST(filePath);
 
                 if (!importedAST) {
                     throw TypeException("Failed to parse imported file: " + filePath);
                 }
             }*/
            // Normal mode: Parse .mt file
            ASTNode* importedAST = importManager->parseAndCacheAST(filePath);

            // Set import evaluation context and evaluate the loaded AST
            env->setImportEvaluation(true);
            evaluateRecursively(importedAST);
            env->setImportEvaluation(false);

            // Mark as evaluated and no longer being evaluated
            importManager->markAsEvaluated(resolvedPath);
            importManager->unmarkAsBeingEvaluated(resolvedPath);

            return std::monostate{}; // Imports return void
        }
        catch (...)
        {
            env->setImportEvaluation(false);
            importManager->unmarkAsBeingEvaluated(resolvedPath);
            throw;
        }
    }

    Value StatementEvaluator::evaluateFunctionNode(FunctionNode* node)
    {
        auto env = context->getEnvironment();


        // Create function definition
        auto funcDef = std::make_shared<FunctionDefinition>(
            node->getName(),
            node->getReturnType(),
            node->getParameters()
        );

        // Set the function body
        funcDef->setBody(node->getBody());

        // Register function in environment
        env->registerFunction(node->getName(), funcDef);


        return std::monostate{}; // Function definitions don't return values
    }

    Value StatementEvaluator::evaluateNativeFunctionNode(NativeFunctionNode* node)
    {
        throw TypeException("Native function evaluation not implemented in refactored version");
    }

    void StatementEvaluator::evaluateRecursively(ASTNode* node)
    {
        if (!node) return;

        // Special handling for ProgramNode - evaluate all its statements
        if (auto programNode = dynamic_cast<ast::nodes::statements::ProgramNode*>(node))
        {
            const auto& statements = programNode->getStatements();
            for (const auto& stmt : statements)
            {
                evaluateRecursively(stmt.get());
            }
            return;
        }

        // Handle different node types by delegating to appropriate evaluators
        if (canHandle(node))
        {
            // This is a statement node - evaluate it ourselves
            evaluate(node);
        }
        else if (exprEvaluator && exprEvaluator->canHandle(node))
        {
            // This is an expression node - delegate to expression evaluator
            exprEvaluator->evaluate(node);
        }
        else if (objEvaluator && objEvaluator->canHandle(node))
        {
            // This is an object node - delegate to object evaluator
            objEvaluator->evaluate(node);
        }
        else
        {
            // Unknown node type - this shouldn't happen in a well-formed AST
            throw TypeException("Unknown node type during import evaluation");
        }
    }

    value::Value StatementEvaluator::evaluateForEachNode(ForEachNode* node)
    {
        // New implementation for mType collections and native arrays
        if (!exprEvaluator)
        {
            throw TypeException("Expression evaluator not available for foreach evaluation");
        }

        // Evaluate the collection to iterate over
        value::Value collectionValue = exprEvaluator->evaluate(node->getCollection());
        auto env = context->getEnvironment();

        // Create a new scope for the loop
        utils::ScopeGuard scope(env, "foreach", environment::manager::ScopeType::BLOCK);

        // Setup loop tracking and control flow
        resetLoopFlags();
        enterLoop(); // Mark that we're entering a loop

        try {
            // Handle native arrays first
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(collectionValue))
        {
            auto array = std::get<std::shared_ptr<value::NativeArray>>(collectionValue);

            for (size_t i = 0; i < array->size(); ++i)
            {
                value::Value element = array->get(i);

                // Define the loop variable in this scope
                auto varType = node->getVariableType();
                auto variableDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
                    node->getVariableName(), varType, element, false, "");

                env->declareVariable(node->getVariableName(), variableDef);

                // Execute the loop body
                if (node->getBody())
                {
                    try
                    {
                        value::Value result = evaluate(node->getBody());

                        // Handle control flow statements
                        if (context->shouldReturn())
                        {
                            exitLoop();
                            return result;
                        }
                    }
                    catch (const BreakException&)
                    {
                        // Break caught - exit foreach loop
                        resetLoopFlags();
                        exitLoop();
                        return std::monostate{};
                    }
                    catch (const ContinueException&)
                    {
                        // Continue caught - reset flags and continue to next iteration
                        resetLoopFlags();
                        continue;
                    }

                    // Check for other control flow interruptions
                    if (flowManager->isBreaking())
                    {
                        break;
                    }
                    if (flowManager->isContinuing())
                    {
                        resetLoopFlags();
                    }
                }
            }
            exitLoop();
            return std::monostate{};
        }

        // Handle mType collection objects
        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(collectionValue))
        {
            auto collection = std::get<std::shared_ptr<ObjectInstance>>(collectionValue);
            auto classDef = collection->getClassDefinition();

            if (!classDef)
            {
                throw ScriptException("Invalid collection object for foreach iteration", node->getLocation());
            }

            std::string className = classDef->getName();

            // Check if this is a collection class by trying to get an array for iteration
            std::shared_ptr<value::NativeArray> iterationArray = nullptr;

            // For Map collections, iterate over values by default
            if (className.find("Map<") == 0)
            {
                // Call getValues() method
                auto getValuesMethod = classDef->findMethod("getValues", 0);
                if (getValuesMethod)
                {
                    // Set current instance context for method call
                    context->setCurrentInstance(collection);

                    // Call getValues() method
                    value::Value valuesResult = objEvaluator->callMethod(collection, "getValues", {});

                    if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(valuesResult))
                    {
                        iterationArray = std::get<std::shared_ptr<value::NativeArray>>(valuesResult);
                    }

                    context->clearCurrentInstance();
                }
            }
            else
            {
                // For other collections (Set, List, Stack, Queue), try toArray() method
                auto toArrayMethod = classDef->findMethod("toArray", 0);
                if (toArrayMethod)
                {
                    // Set current instance context for method call
                    context->setCurrentInstance(collection);

                    // Call toArray() method
                    value::Value arrayResult = objEvaluator->callMethod(collection, "toArray", {});

                    if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(arrayResult))
                    {
                        iterationArray = std::get<std::shared_ptr<value::NativeArray>>(arrayResult);
                    }

                    context->clearCurrentInstance();
                }
            }

            if (!iterationArray)
            {
                throw ScriptException(
                    "Collection '" + className +
                    "' does not support iteration (missing toArray() or getValues() method)", node->getLocation());
            }

            // Iterate through the array
            for (size_t i = 0; i < iterationArray->size(); ++i)
            {
                value::Value element = iterationArray->get(i);

                // Define the loop variable in this scope
                auto varType = node->getVariableType();
                auto variableDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
                    node->getVariableName(), varType, element, false, "");

                env->declareVariable(node->getVariableName(), variableDef);

                // Execute the loop body
                if (node->getBody())
                {
                    try
                    {
                        value::Value result = evaluate(node->getBody());

                        // Handle control flow statements
                        if (context->shouldReturn())
                        {
                            exitLoop();
                            return result;
                        }
                    }
                    catch (const BreakException&)
                    {
                        // Break caught - exit foreach loop
                        resetLoopFlags();
                        exitLoop();
                        return std::monostate{};
                    }
                    catch (const ContinueException&)
                    {
                        // Continue caught - reset flags and continue to next iteration
                        resetLoopFlags();
                        continue;
                    }

                    // Check for other control flow interruptions
                    if (flowManager->isBreaking())
                    {
                        break;
                    }
                    if (flowManager->isContinuing())
                    {
                        resetLoopFlags();
                    }
                }
            }
            exitLoop();
            return std::monostate{};
        }

            throw ScriptException("Value is not a valid collection for foreach iteration", node->getLocation());
        }
        catch (const BreakException&)
        {
            // Break caught at foreach level
            resetLoopFlags();
        }
        catch (...)
        {
            // Any other exception - make sure to clean up loop state
            exitLoop();
            throw;
        }

        // Normal completion - clean up loop state
        exitLoop();
        return std::monostate{};
    }

    Value StatementEvaluator::convertLambdaToInterface(const Value& lambdaValue, const std::string& interfaceName)
    {
        // Extract the lambda value
        auto lambdaPtr = std::get<std::shared_ptr<value::LambdaValue>>(lambdaValue);
        auto* lambdaNode = lambdaPtr->getLambda();

        // Get the interface definition from the environment
        auto env = context->getEnvironment();
        auto interfaceDef = env->findInterface(interfaceName);

        if (!interfaceDef) {
            return lambdaValue;
        }

        // Check if the interface is functional (has exactly one method)
        if (!interfaceDef->isFunctionalInterface()) {
            return lambdaValue;
        }

        // Create the lambda implementation class
        auto implClass = interfaceDef->createLambdaImplementation(lambdaNode);
        if (!implClass) {
            return lambdaValue;
        }

        // Create an instance of the implementation class
        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(implClass);

        // Store the lambda in a special field that the implementation can access
        instance->setField("__lambda", lambdaValue);

        return instance;
    }
}
