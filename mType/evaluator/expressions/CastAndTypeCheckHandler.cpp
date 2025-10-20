#include "CastAndTypeCheckHandler.hpp"
#include "../../ast/nodes/expressions/CastExpression.hpp"
#include "../../ast/nodes/expressions/InstanceOfExpression.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../errors/TypeConversionException.hpp"
#include "../../value/ValueType.hpp"
#include "../../value/StringPool.hpp"
#include "../utils/ValueConverter.hpp"

using namespace errors;
using namespace runtimeTypes::klass;
using evaluator::utils::ValueConverter;

namespace evaluator
{
    namespace expressions
    {
        CastAndTypeCheckHandler::CastAndTypeCheckHandler(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr)
        {
        }

        void CastAndTypeCheckHandler::setExpressionEvaluator(interfaces::IExpressionEvaluator* evaluator)
        {
            exprEvaluator = evaluator;
        }

        Value CastAndTypeCheckHandler::evaluateCast(CastExpression* node)
        {
            // Defensive check - exprEvaluator should be set by ExpressionEvaluator
            if (!exprEvaluator) {
                throw std::runtime_error(
                    "CastAndTypeCheckHandler::evaluateCast: "
                    "ExpressionEvaluator not initialized. "
                    "Check ExpressionEvaluator constructor.");
            }

            // Evaluate the expression to cast
            Value sourceValue = exprEvaluator->evaluate(node->getExpression());
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
            ValueType sourceValueType = value::ValueTypeUtils::getValueType(sourceValue);

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
                    "Cannot cast null to primitive type",
                    "null",
                    targetTypeName,
                    node->getLocation()
                );
            }

            // Primitive to primitive casting
            if (sourceValueType != ValueType::OBJECT && isPrimitiveTarget) {
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

        Value CastAndTypeCheckHandler::evaluateInstanceOf(InstanceOfExpression* node)
        {
            // Defensive check - exprEvaluator should be set by ExpressionEvaluator
            if (!exprEvaluator) {
                throw std::runtime_error(
                    "CastAndTypeCheckHandler::evaluateInstanceOf: "
                    "ExpressionEvaluator not initialized. "
                    "Check ExpressionEvaluator constructor.");
            }

            // Evaluate the expression to check
            Value value = exprEvaluator->evaluate(node->getExpression());
            auto targetType = node->getTargetType();
            std::string targetTypeName = targetType->toString();

            // null isClassOf anything is false
            if (std::holds_alternative<std::monostate>(value)) {
                return false;
            }

            ValueType valueType = value::ValueTypeUtils::getValueType(value);

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

        Value CastAndTypeCheckHandler::castPrimitive(const Value& value, ValueType targetType, const std::string& targetTypeName, const errors::SourceLocation& location)
        {
            ValueType sourceType = value::ValueTypeUtils::getValueType(value);

            // Same type - no conversion needed
            if (sourceType == targetType) {
                return value;
            }

            switch (targetType) {
            case ValueType::INT:
                return exprEvaluator->toInt(value);
            case ValueType::FLOAT:
                return exprEvaluator->toFloat(value);
            case ValueType::BOOL:
                return exprEvaluator->isTruthy(value);
            case ValueType::STRING: {
                auto& pool = value::StringPool::getInstance();
                return pool.intern(exprEvaluator->toString(value));
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

        Value CastAndTypeCheckHandler::castObject(const Value& value, const std::string& targetClassName, const errors::SourceLocation& location)
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

        bool CastAndTypeCheckHandler::isInstanceOfClass(std::shared_ptr<ObjectInstance> objInstance, const std::string& targetClassName)
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
}
