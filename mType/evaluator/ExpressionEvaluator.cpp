#include "ExpressionEvaluator.hpp"
#include "expressions/LiteralEvaluator.hpp"
#include "expressions/BinaryOperationEvaluator.hpp"
#include "expressions/CallHandler.hpp"
#include "expressions/ArrayHandler.hpp"
#include "expressions/UnaryOperationHandler.hpp"
#include "expressions/AccessHandler.hpp"
#include "utils/ValueConverter.hpp"
#include "utils/NodeTypeRegistry.hpp"
#include "utils/ParameterBinder.hpp"
#include "../value/StringPool.hpp"
#include "../value/LambdaValue.hpp"
#include "../value/PromiseValue.hpp"
#include "../ast/nodes/expressions/LambdaNode.hpp"
#include "../ast/nodes/expressions/CastExpression.hpp"
#include "../ast/nodes/expressions/InstanceOfExpression.hpp"
#include "../errors/TypeException.hpp"
#include "../errors/UndefinedException.hpp"
#include "../errors/TypeConversionException.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../ast/nodes/expressions/VariableNode.hpp"
#include "../ast/nodes/expressions/IntegerNode.hpp"
#include "../ast/nodes/expressions/FloatNode.hpp"
#include "../ast/nodes/expressions/StringNode.hpp"
#include "../ast/nodes/expressions/BoolNode.hpp"
#include "../ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../value/ArrayPool.hpp"
#include "../parser/TypeParser.hpp"
#include "../ast/nodes/functions/FunctionCallNode.hpp"
#include "../ast/nodes/classes/MemberAccessNode.hpp"
#include "../ast/nodes/classes/MethodCallNode.hpp"
#include "../ast/nodes/classes/SuperConstructorCallNode.hpp"
#include "../ast/nodes/classes/SuperMethodCallNode.hpp"
#include "../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/classes/NewNode.hpp"
#include "ObjectEvaluator.hpp"
#include "StatementEvaluator.hpp"
#include "../errors/ReturnException.hpp"


namespace evaluator
{
    using namespace errors;
    using namespace runtimeTypes;
    using namespace runtimeTypes::global;
    using namespace runtimeTypes::klass;
    using namespace value;

    ExpressionEvaluator::ExpressionEvaluator(std::shared_ptr<EvaluationContext> ctx)
        : context(ctx)
          , literalEvaluator(std::make_unique<expressions::LiteralEvaluator>(ctx))
          , binaryOpEvaluator(std::make_unique<expressions::BinaryOperationEvaluator>(ctx))
          , callHandler(std::make_unique<expressions::CallHandler>(ctx))
          , arrayHandler(std::make_unique<expressions::ArrayHandler>(ctx))
          , unaryOpHandler(std::make_unique<expressions::UnaryOperationHandler>(ctx))
          , accessHandler(std::make_unique<expressions::AccessHandler>(ctx))
          , stmtEvaluator(nullptr)
          , objEvaluator(nullptr)
    {
        // Set back-references
        binaryOpEvaluator->setExpressionEvaluator(this);
        callHandler->setExpressionEvaluator(this);
        arrayHandler->setExpressionEvaluator(this);
        unaryOpHandler->setExpressionEvaluator(this);
        accessHandler->setExpressionEvaluator(this);

        // Initialize dispatcher with all node type handlers
        initializeDispatcher();
    }

    // Destructor must be defined in .cpp where complete types are available
    ExpressionEvaluator::~ExpressionEvaluator() = default;

    Value ExpressionEvaluator::evaluate(ASTNode* node)
    {
        if (!node || !canHandle(node))
        {
            return std::monostate{};
        }

        // Use dispatcher for O(1) lookup instead of O(n) dynamic_cast chain
        return dispatcher.dispatch(this, node);
    }

    void ExpressionEvaluator::initializeDispatcher()
    {
        // Register all expression node handlers with the dispatcher
        dispatcher.registerMethod<IntegerNode>(&ExpressionEvaluator::evaluateIntegerNode);
        dispatcher.registerMethod<FloatNode>(&ExpressionEvaluator::evaluateFloatNode);
        dispatcher.registerMethod<StringNode>(&ExpressionEvaluator::evaluateStringNode);
        dispatcher.registerMethod<BoolNode>(&ExpressionEvaluator::evaluateBoolNode);
        dispatcher.registerMethod<NullNode>(&ExpressionEvaluator::evaluateNullNode);
        dispatcher.registerMethod<LambdaNode>(&ExpressionEvaluator::evaluateLambdaNode);
        dispatcher.registerMethod<VariableNode>(&ExpressionEvaluator::evaluateVariableNode);
        dispatcher.registerMethod<BinaryExpNode>(&ExpressionEvaluator::evaluateBinaryExpNode);
        dispatcher.registerMethod<TernaryExpNode>(&ExpressionEvaluator::evaluateTernaryExpNode);
        dispatcher.registerMethod<UnaryExpNode>(&ExpressionEvaluator::evaluateUnaryExpNode);
        dispatcher.registerMethod<FunctionCallNode>(&ExpressionEvaluator::evaluateFunctionCallNode);
        dispatcher.registerMethod<AssignmentNode>(&ExpressionEvaluator::evaluateAssignmentExpression);
        dispatcher.registerMethod<ArrayCreationNode>(&ExpressionEvaluator::evaluateArrayCreationNode);
        dispatcher.registerMethod<ArrayLiteralNode>(&ExpressionEvaluator::evaluateArrayLiteralNode);
        dispatcher.registerMethod<IndexAccessNode>(&ExpressionEvaluator::evaluateIndexAccessNode);
        dispatcher.registerMethod<LambdaInterfaceInvocationNode>(&ExpressionEvaluator::evaluateLambdaInterfaceInvocationNode);

        // Object-related nodes that can appear in expressions
        dispatcher.registerMethod<MemberAccessNode>(&ExpressionEvaluator::evaluateMemberAccessNode);
        dispatcher.registerMethod<MethodCallNode>(&ExpressionEvaluator::evaluateMethodCallNode);
        dispatcher.registerMethod<NewNode>(&ExpressionEvaluator::evaluateNewNode);
        dispatcher.registerMethod<SuperConstructorCallNode>(&ExpressionEvaluator::evaluateSuperConstructorCallNode);
        dispatcher.registerMethod<SuperMethodCallNode>(&ExpressionEvaluator::evaluateSuperMethodCallNode);

        // Cast and type checking expressions
        dispatcher.registerMethod<CastExpression>(&ExpressionEvaluator::evaluateCastExpression);
        dispatcher.registerMethod<InstanceOfExpression>(&ExpressionEvaluator::evaluateInstanceOfExpression);

        // Async/await expressions
        dispatcher.registerMethod<AwaitExpression>(&ExpressionEvaluator::evaluateAwaitExpression);

        // Special case: MemberAssignmentNode delegates to ObjectEvaluator
        dispatcher.registerHandler<MemberAssignmentNode>([this](ExpressionEvaluator* eval, ASTNode* node) {
            if (objEvaluator)
            {
                return objEvaluator->evaluate(node);
            }
            throw UndefinedException("Object evaluator not available for member assignment", node->getLocation());
        });
    }

    bool ExpressionEvaluator::canHandle(ASTNode* node) const
    {
        return isExpressionNode(node);
    }

    bool ExpressionEvaluator::isTruthy(const Value& value) const
    {
        return ValueConverter::isTruthy(value);
    }

    std::string ExpressionEvaluator::toString(const Value& value) const
    {
        // Handle objects with potential toString() method
        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(value))
        {
            auto objectInstance = std::get<std::shared_ptr<ObjectInstance>>(value);
            if (!objectInstance)
            {
                return "null";
            }

            // Try to call toString() method if it exists
            auto classDef = objectInstance->getClassDefinition();
            if (classDef && classDef->hasMethod("toString"))
            {
                auto toStringMethod = classDef->findMethod("toString", 0);
                if (toStringMethod && !toStringMethod->isStatic())
                {
                    try
                    {
                        Value result = objEvaluator->callMethod(objectInstance, "toString", {});
                        if (std::holds_alternative<std::string>(result))
                        {
                            return std::get<std::string>(result);
                        }
                        else if (std::holds_alternative<InternedString>(result))
                        {
                            return std::get<InternedString>(result).getString();
                        }
                    }
                    catch (...)
                    {
                        // If toString() fails, fall back to default representation
                    }
                }
            }
        }

        // Fall back to ValueConverter for all other types or when toString() is not available
        return ValueConverter::toString(value);
    }

    float ExpressionEvaluator::toFloat(const Value& value) const
    {
        return ValueConverter::toFloat(value);
    }

    int ExpressionEvaluator::toInt(const Value& value) const
    {
        return ValueConverter::toInt(value);
    }

    void ExpressionEvaluator::setStatementEvaluator(StatementEvaluator* evaluator)
    {
        stmtEvaluator = evaluator;
        callHandler->setStatementEvaluator(evaluator);
        accessHandler->setStatementEvaluator(evaluator);
    }

    void ExpressionEvaluator::setObjectEvaluator(ObjectEvaluator* evaluator)
    {
        objEvaluator = evaluator;
        callHandler->setObjectEvaluator(evaluator);
        unaryOpHandler->setObjectEvaluator(evaluator);
        accessHandler->setObjectEvaluator(evaluator);
    }

    bool ExpressionEvaluator::isExpressionNode(ASTNode* node) const
    {
        // Use NodeTypeRegistry for O(1) lookup instead of O(n) dynamic_cast chain
        return utils::NodeTypeRegistry::isExpression(node);
    }

    Value ExpressionEvaluator::evaluateIntegerNode(IntegerNode* node)
    {
        return literalEvaluator->evaluateInteger(node);
    }

    Value ExpressionEvaluator::evaluateFloatNode(FloatNode* node)
    {
        return literalEvaluator->evaluateFloat(node);
    }

    Value ExpressionEvaluator::evaluateStringNode(StringNode* node)
    {
        return literalEvaluator->evaluateString(node);
    }

    Value ExpressionEvaluator::evaluateBoolNode(BoolNode* node)
    {
        return literalEvaluator->evaluateBool(node);
    }

    Value ExpressionEvaluator::evaluateNullNode(NullNode* node)
    {
        return literalEvaluator->evaluateNull(node);
    }

    Value ExpressionEvaluator::evaluateVariableNode(VariableNode* node)
    {
        return accessHandler->evaluateVariableAccess(node);
    }

    Value ExpressionEvaluator::evaluateBinaryExpNode(BinaryExpNode* node)
    {
        Value left = evaluate(node->getLeft());
        TokenType op = node->getOperator();

        // Handle short-circuit evaluation for logical operators
        if (op == TokenType::AND)
        {
            if (!isTruthy(left))
            {
                return false;
            }
            Value right = evaluate(node->getRight());
            return isTruthy(right);
        }
        else if (op == TokenType::OR)
        {
            if (isTruthy(left))
            {
                return true;
            }
            Value right = evaluate(node->getRight());
            return isTruthy(right);
        }

        Value right = evaluate(node->getRight());

        // Handle different operator categories
        switch (op)
        {
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
        Value condition = evaluate(node->getCondition());

        if (isTruthy(condition))
        {
            return evaluate(node->getTrueExpression());
        }
        else
        {
            return evaluate(node->getFalseExpression());
        }
    }

    Value ExpressionEvaluator::evaluateUnaryExpNode(UnaryExpNode* node)
    {
        return unaryOpHandler->evaluateUnaryOperation(node);
    }

    Value ExpressionEvaluator::evaluateFunctionCallNode(FunctionCallNode* node)
    {
        return callHandler->evaluateFunctionCall(node);
    }

    Value ExpressionEvaluator::evaluateMemberAccessNode(MemberAccessNode* node)
    {
        return accessHandler->evaluateMemberAccess(node);
    }

    Value ExpressionEvaluator::evaluateMethodCallNode(MethodCallNode* node)
    {
        return callHandler->evaluateMethodCall(node);
    }

    Value ExpressionEvaluator::evaluateNewNode(NewNode* node)
    {
        return accessHandler->evaluateObjectCreation(node);
    }

    // Delegate binary operations to BinaryOperationEvaluator
    Value ExpressionEvaluator::evaluateArithmetic(const Value& left, const Value& right, TokenType op)
    {
        return binaryOpEvaluator->evaluateArithmetic(left, right, op);
    }

    Value ExpressionEvaluator::evaluateComparison(const Value& left, const Value& right, TokenType op)
    {
        return binaryOpEvaluator->evaluateComparison(left, right, op);
    }

    Value ExpressionEvaluator::evaluateLogical(const Value& left, const Value& right, TokenType op)
    {
        return binaryOpEvaluator->evaluateLogical(left, right, op);
    }

    Value ExpressionEvaluator::evaluateStringOperation(const Value& left, const Value& right, TokenType op)
    {
        return binaryOpEvaluator->evaluateStringOperation(left, right, op);
    }

    Value ExpressionEvaluator::evaluateAssignmentExpression(AssignmentNode* node)
    {
        return accessHandler->evaluateAssignment(node);
    }


    // Helper function to get default value for a given type
    Value ExpressionEvaluator::getDefaultValueForType(const ::parser::TypeInfo& elementType)
    {
        return arrayHandler->getDefaultValueForType(elementType);
    }

    // Collection-related method implementations
    Value ExpressionEvaluator::evaluateArrayCreationNode(ArrayCreationNode* node)
    {
        return arrayHandler->evaluateArrayCreation(node);
    }

    Value ExpressionEvaluator::evaluateArrayLiteralNode(ArrayLiteralNode* node)
    {
        return arrayHandler->evaluateArrayLiteral(node);
    }

    Value ExpressionEvaluator::evaluateIndexAccessNode(IndexAccessNode* node)
    {
        return arrayHandler->evaluateIndexAccess(node);
    }

    std::optional<Value> ExpressionEvaluator::extractMultiDimensionalAccess(
        IndexAccessNode* node, std::vector<size_t>& indices)
    {
        return arrayHandler->extractMultiDimensionalAccess(node, indices);
    }

    Value ExpressionEvaluator::evaluateDirectMultiDimensionalAccess(const Value& baseArray,
                                                                    const std::vector<size_t>& indices,
                                                                    const SourceLocation& location)
    {
        return arrayHandler->evaluateDirectMultiDimensionalAccess(baseArray, indices, location);
    }

    Value ExpressionEvaluator::evaluateLambdaNode(LambdaNode* node)
    {
        // Create a LambdaValue with the current evaluation context
        // The LambdaValue will capture the current environment for closure support
        auto lambdaValue = std::make_shared<value::LambdaValue>(node, context);

        return lambdaValue;
    }

    Value ExpressionEvaluator::evaluateLambdaInterfaceInvocationNode(LambdaInterfaceInvocationNode* node)
    {
        return callHandler->evaluateLambdaInterfaceInvocation(node);
    }

    Value ExpressionEvaluator::evaluateSuperConstructorCallNode(SuperConstructorCallNode* node)
    {
        // Check if we're being called from initializer (allowed) or body (not allowed)
        // If we're in an initializer context, the context flag will be set
        if (!context->isInSuperInitializerContext()) {
            throw UndefinedException(
                "super() cannot be called in constructor body. Use initializer list syntax: constructor(...) : super(...)",
                node->getLocation());
        }

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
            argValues.push_back(evaluate(arg.get()));
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
                    evaluate(static_cast<ASTNode*>(parentSuperInit));

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

    Value ExpressionEvaluator::evaluateSuperMethodCallNode(SuperMethodCallNode* node)
    {
        // Get current instance - super.method() can only be called in instance methods
        auto currentInstance = context->getCurrentInstance();
        if (!currentInstance) {
            throw UndefinedException(
                "super." + node->getMethodName() + "() can only be called within an instance method",
                node->getLocation());
        }

        auto currentClass = currentInstance->getClassDefinition();
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
            argValues.push_back(evaluate(arg.get()));
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
                    result = e.returnValue;
                    context->setReturned(false);  // Reset return flag
                }
            }

            context->getEnvironment()->exitScope();
            return result;
        }

        throw UndefinedException(
            "Object evaluator not available for super." + node->getMethodName() + "() call",
            node->getLocation());
    }

    Value ExpressionEvaluator::evaluateCastExpression(CastExpression* node)
    {
        // Evaluate the expression to cast
        Value sourceValue = evaluate(node->getExpression());
        auto targetType = node->getTargetType();

        // Get target type information
        std::string targetTypeName = targetType->toString();
        ValueType targetValueType = ValueType::VOID;

        // Determine target value type by checking if it's a concrete primitive type
        bool isPrimitiveTarget = !targetType->isGenericParameter();
        if (isPrimitiveTarget) {
            try {
                targetValueType = targetType->getConcreteType();
                // Check if this is actually a primitive (not OBJECT, ARRAY, etc.)
                isPrimitiveTarget = (targetValueType == ValueType::INT ||
                                      targetValueType == ValueType::FLOAT ||
                                      targetValueType == ValueType::BOOL ||
                                      targetValueType == ValueType::STRING);
                if (!isPrimitiveTarget) {
                    targetValueType = ValueType::OBJECT;
                }
            }
            catch (...) {
                // Not a concrete type, treat as object
                targetValueType = ValueType::OBJECT;
            }
        } else {
            targetValueType = ValueType::OBJECT;
        }

        // Handle null - null can be cast to any object type but not primitives
        ValueType sourceValueType = value::getValueType(sourceValue);

        bool isSourceNull = (sourceValueType == ValueType::NULL_TYPE) ||
                            std::holds_alternative<std::monostate>(sourceValue) ||
                            std::holds_alternative<nullptr_t>(sourceValue);

        // Also check for nullptr shared_ptr (null object reference)
        if (!isSourceNull && std::holds_alternative<std::shared_ptr<ObjectInstance>>(sourceValue)) {
            auto objPtr = std::get<std::shared_ptr<ObjectInstance>>(sourceValue);
            isSourceNull = (objPtr == nullptr);
        }

        if (isSourceNull) {
            if (targetValueType == ValueType::OBJECT) {
                return sourceValue; // null remains null
            }
            throw TypeConversionException(
                "Cannot cast null to primitive type " + targetTypeName,
                "null",
                targetTypeName,
                node->getLocation()
            );
        }

        // Primitive to primitive casting
        if (sourceValueType != ValueType::OBJECT && targetValueType != ValueType::OBJECT) {
            return castPrimitive(sourceValue, targetValueType, targetTypeName, node->getLocation());
        }

        // Object to object casting
        if (sourceValueType == ValueType::OBJECT && targetValueType == ValueType::OBJECT) {
            return castObject(sourceValue, targetTypeName, node->getLocation());
        }

        // Cross-category cast not allowed
        throw TypeConversionException(
            "Cannot cast between primitive and object types",
            ValueConverter::valueTypeToString(sourceValueType),
            targetTypeName,
            node->getLocation()
        );
    }

    Value ExpressionEvaluator::evaluateInstanceOfExpression(InstanceOfExpression* node)
    {
        // Evaluate the expression to check
        Value value = evaluate(node->getExpression());
        auto targetType = node->getTargetType();
        std::string targetTypeName = targetType->toString();

        // null isClassOf anything is false
        if (std::holds_alternative<std::monostate>(value)) {
            return false;
        }

        ValueType valueType = value::getValueType(value);

        // For primitives, check exact type match
        if (valueType != ValueType::OBJECT) {
            bool isMatch = false;
            if (valueType == ValueType::INT && targetTypeName == "int") isMatch = true;
            else if (valueType == ValueType::FLOAT && targetTypeName == "float") isMatch = true;
            else if (valueType == ValueType::BOOL && targetTypeName == "bool") isMatch = true;
            else if (valueType == ValueType::STRING && targetTypeName == "string") isMatch = true;

            return isMatch;
        }

        // For objects, check inheritance and interfaces
        auto objInstance = std::get<std::shared_ptr<ObjectInstance>>(value);
        bool isInstance = isInstanceOfClass(objInstance, targetTypeName);

        return isInstance;
    }

    Value ExpressionEvaluator::evaluateAwaitExpression(AwaitExpression* node)
    {
        // Evaluate the expression being awaited
        Value awaitedValue = evaluate(node->getExpressionPtr());

        // Check if the value is a Promise
        if (!std::holds_alternative<std::shared_ptr<PromiseValue>>(awaitedValue))
        {
            throw std::runtime_error("await can only be used on Promise values");
        }

        // Get the promise
        auto promise = std::get<std::shared_ptr<PromiseValue>>(awaitedValue);

        // Defensive null check
        if (!promise)
        {
            throw std::runtime_error("Null promise in await expression");
        }

        // In Phase 2 (synchronous model), await immediately returns the promise's value
        // The promise should already be fulfilled when returned from an async function
        if (!promise->isFulfilled())
        {
            throw std::runtime_error("Promise is not fulfilled");
        }

        // Return the unwrapped value
        return promise->getValue();
    }

    Value ExpressionEvaluator::castPrimitive(const Value& value, ValueType targetType, const std::string& targetTypeName, const SourceLocation& location)
    {
        ValueType sourceType = value::getValueType(value);

        // Same type - no conversion needed
        if (sourceType == targetType) {
            return value;
        }

        switch (targetType) {
        case ValueType::INT:
            return toInt(value);
        case ValueType::FLOAT:
            return toFloat(value);
        case ValueType::BOOL:
            return isTruthy(value);
        case ValueType::STRING: {
            auto& pool = value::StringPool::getInstance();
            return pool.intern(toString(value));
        }
        default:
            throw TypeConversionException(
                "Invalid primitive cast target type",
                ValueConverter::valueTypeToString(sourceType),
                targetTypeName,
                location
            );
        }
    }

    Value ExpressionEvaluator::castObject(const Value& value, const std::string& targetClassName, const SourceLocation& location)
    {
        auto objInstance = std::get<std::shared_ptr<ObjectInstance>>(value);

        // Check if cast is valid using isInstanceOf logic
        if (isInstanceOfClass(objInstance, targetClassName)) {
            return value; // Cast succeeds, return same object
        }

        // Cast failed
        throw TypeConversionException(
            "Cannot cast object to incompatible type",
            objInstance->getTypeName(),
            targetClassName,
            location
        );
    }

    bool ExpressionEvaluator::isInstanceOfClass(std::shared_ptr<ObjectInstance> objInstance, const std::string& targetClassName)
    {
        if (!objInstance) {
            return false;
        }

        auto classDef = objInstance->getClassDefinition();
        if (!classDef) {
            return false;
        }

        std::string actualClassName = classDef->getName();

        // Extract base class name from generic types (e.g., "Box<int>" -> "Box")
        auto extractBaseName = [](const std::string& name) -> std::string {
            size_t anglePos = name.find('<');
            return (anglePos != std::string::npos) ? name.substr(0, anglePos) : name;
        };

        std::string actualBaseName = extractBaseName(actualClassName);
        std::string targetBaseName = extractBaseName(targetClassName);

        // Exact match (either full name or base name)
        if (actualClassName == targetClassName || actualBaseName == targetBaseName) {
            return true;
        }

        // Check inheritance chain (upcast: Child → Parent)
        // Try both full name and base name for compatibility
        if (classDef->isSubclassOf(targetClassName) || classDef->isSubclassOf(targetBaseName)) {
            return true;
        }

        // Check interface implementation
        auto interfaceRegistry = context->getEnvironment()->getInterfaceRegistry();
        if (classDef->implementsInterface(targetClassName, interfaceRegistry) ||
            classDef->implementsInterface(targetBaseName, interfaceRegistry)) {
            return true;
        }

        return false;
    }
}
