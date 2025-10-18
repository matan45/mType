#include "TypeValidator.hpp"
#include "../utils/ValueConverter.hpp"
#include "../utils/GenericTypeManager.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/UndefinedException.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../value/NativeArray.hpp"
#include "../../value/FlatMultiArray.hpp"
#include "../../value/arrays/object/FlatMultiObjectArray.hpp"
#include "../../value/PromiseValue.hpp"
#include <variant>
#include <unordered_map>

namespace evaluator
{
    namespace validation
    {
        using namespace utils;

        void TypeValidator::validateAssignment(
            ValueType expectedType,
            const Value& actualValue,
            const std::string& variableName,
            const SourceLocation& location)
        {
            // Skip validation for void initializers (default values)
            if (std::holds_alternative<std::monostate>(actualValue) && expectedType != ValueType::VOID)
            {
                return; // Allow default initialization
            }

            ValueType actualType = ValueConverter::getValueType(actualValue);

            // Allow null assignment to object types
            if (actualType == ValueType::NULL_TYPE && expectedType == ValueType::OBJECT)
            {
                return;
            }

            // Allow Promise values for object types (async/await support)
            // Promise types will be validated when awaited
            if (std::holds_alternative<std::shared_ptr<value::PromiseValue>>(actualValue) &&
                expectedType == ValueType::OBJECT)
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
            throw TypeException(
                "Type mismatch for variable '" + variableName + "': expected " +
                ValueConverter::valueTypeToString(expectedType) +
                " but got " + ValueConverter::valueTypeToString(actualType),
                location);
        }

        void TypeValidator::validateAssignment(
            ValueType expectedType,
            const Value& actualValue,
            const std::string& variableName,
            const SourceLocation& location,
            const std::string& expectedClassName,
            std::shared_ptr<EvaluationContext> context)
        {
            // Skip validation for void initializers (default values)
            if (std::holds_alternative<std::monostate>(actualValue) && expectedType != ValueType::VOID)
            {
                return; // Allow default initialization
            }

            ValueType actualType = ValueConverter::getValueType(actualValue);

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
                    validateObjectTypeCompatibility(actualValue, variableName, location, expectedClassName, context);
                }
                return;
            }

            // Allow valid implicit conversions
            if (isValidTypeConversion(actualType, expectedType))
            {
                return;
            }

            // Type mismatch
            throw TypeException(
                "Type mismatch for variable '" + variableName + "': expected " +
                ValueConverter::valueTypeToString(expectedType) +
                " but got " + ValueConverter::valueTypeToString(actualType),
                location);
        }

        void TypeValidator::validateFunctionReturn(
            ValueType expectedType,
            const Value& returnValue,
            const std::string& functionName,
            const SourceLocation& location)
        {
            ValueType actualType = ValueConverter::getValueType(returnValue);

            // Allow null return for object types
            if (actualType == ValueType::NULL_TYPE && expectedType == ValueType::OBJECT)
            {
                return;
            }

            // Allow void returns (monostate) for void functions
            if (actualType == ValueType::VOID && expectedType == ValueType::VOID)
            {
                return;
            }

            // Check for type mismatch
            if (actualType != expectedType)
            {
                throw TypeException(
                    "Return type mismatch in function '" + functionName + "': expected " +
                    ValueConverter::valueTypeToString(expectedType) +
                    " but got " + ValueConverter::valueTypeToString(actualType),
                    location);
            }
        }

        void TypeValidator::validateFunctionReturn(
            ValueType expectedType,
            const Value& returnValue,
            const std::string& functionName,
            const SourceLocation& location,
            const std::string& returnClassName,
            bool isAsync)
        {
            ValueType actualType = ValueConverter::getValueType(returnValue);

            // Allow null return for object types
            if (actualType == ValueType::NULL_TYPE && expectedType == ValueType::OBJECT)
            {
                return;
            }

            // Allow void returns (monostate) for void functions
            if (actualType == ValueType::VOID && expectedType == ValueType::VOID)
            {
                return;
            }

            // Special case: async functions with Promise<void> can return void
            // The AsyncReturnGuard will wrap the void value in a Promise
            if (isAsync && returnClassName == "Promise<void>" &&
                actualType == ValueType::VOID && expectedType == ValueType::OBJECT)
            {
                return;
            }

            // Check for type mismatch
            if (actualType != expectedType)
            {
                throw TypeException(
                    "Return type mismatch in function '" + functionName + "': expected " +
                    ValueConverter::valueTypeToString(expectedType) +
                    " but got " + ValueConverter::valueTypeToString(actualType),
                    location);
            }
        }

        void TypeValidator::validateClassExists(
            const std::string& className,
            const SourceLocation& location,
            std::shared_ptr<EvaluationContext> context)
        {
            auto env = context->getEnvironment();

            // Resolve generic type parameters if needed
            std::string resolvedClassName = resolveGenericClassName(className, context);

            // Check if the class or interface is defined
            if (!env->findClass(resolvedClassName) && !env->findInterface(resolvedClassName))
            {
                throw UndefinedException("Class '" + resolvedClassName + "' is not defined", location);
            }
        }

        void TypeValidator::validateObjectTypeCompatibility(
            const Value& value,
            const std::string& variableName,
            const SourceLocation& location,
            const std::string& expectedClassName,
            std::shared_ptr<EvaluationContext> context)
        {
            // Handle Object instances
            if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(value))
            {
                auto objInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(value);
                if (objInstance)
                {
                    std::string actualClassName = objInstance->getClassDefinition()->getName();

                    // Resolve generic type parameters in expected class name
                    std::string resolvedExpectedClassName = resolveGenericClassName(expectedClassName, context);

                    // Check if the actual class matches the expected class
                    if (!isGenericTypeCompatible(actualClassName, resolvedExpectedClassName))
                    {
                        // Check if actualClassName is a subclass of expectedClassName (inheritance)
                        if (!isSubclassCompatible(actualClassName, resolvedExpectedClassName, context))
                        {
                            // Check if expectedClassName is an interface that the actual class implements
                            auto classDefinition = objInstance->getClassDefinition();

                            // Extract base interface name from generic type
                            std::string baseExpectedName = extractBaseClassName(resolvedExpectedClassName);

                            // Get the interface registry from the environment
                            if (context)
                            {
                                auto interfaceRegistry = context->getEnvironment()->getInterfaceRegistry();

                                if (!classDefinition->implementsInterface(baseExpectedName, interfaceRegistry))
                                {
                                    throw TypeException(
                                        "Object type mismatch for variable '" + variableName + "': expected " +
                                        resolvedExpectedClassName + " but got " + actualClassName,
                                        location);
                                }
                            }
                        }
                    }
                }
                return;
            }

            // Handle NativeArray (1D arrays)
            if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(value))
            {
                if (expectedClassName.find("[]") != std::string::npos)
                {
                    return; // Array type matches
                }
                else
                {
                    throw TypeException(
                        "Type mismatch for variable '" + variableName + "': expected " +
                        expectedClassName + " but got array type",
                        location);
                }
            }

            // Handle FlatMultiArray (multi-dimensional arrays)
            if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(value))
            {
                if (expectedClassName.find("[]") != std::string::npos)
                {
                    return; // Multi-dimensional array type matches
                }
                else
                {
                    throw TypeException(
                        "Type mismatch for variable '" + variableName + "': expected " +
                        expectedClassName + " but got multi-dimensional array type",
                        location);
                }
            }

            // Handle FlatMultiObjectArray (multi-dimensional object arrays with SoA)
            if (std::holds_alternative<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(value))
            {
                auto flatMultiObjectArray = std::get<std::shared_ptr<
                    mType::value::arrays::FlatMultiObjectArray>>(value);

                if (expectedClassName.find("[]") != std::string::npos)
                {
                    return; // Multi-dimensional object array type matches
                }
                else
                {
                    throw TypeException(
                        "Type mismatch for variable '" + variableName + "': expected " +
                        expectedClassName + " but got multi-dimensional object array type",
                        location);
                }
            }

            // Handle PromiseValue (async/await promises)
            if (std::holds_alternative<std::shared_ptr<value::PromiseValue>>(value))
            {
                // Extract base class name from generic type (e.g., "Promise<Int>" -> "Promise")
                std::string baseExpectedName = extractBaseClassName(expectedClassName);

                if (baseExpectedName == "Promise")
                {
                    // Promise types are compatible - the generic parameter will be checked when awaited
                    return;
                }
                else
                {
                    throw TypeException(
                        "Type mismatch for variable '" + variableName + "': expected " +
                        expectedClassName + " but got Promise type",
                        location);
                }
            }
        }

        bool TypeValidator::isValidTypeConversion(ValueType from, ValueType to)
        {
            // Allow int to float conversion (common implicit conversion)
            if (from == ValueType::INT && to == ValueType::FLOAT)
            {
                return true;
            }

            // No other conversions allowed for now
            return false;
        }

        bool TypeValidator::isGenericTypeCompatible(
            const std::string& actualClassName,
            const std::string& expectedClassName)
        {
            // Exact match is always compatible
            if (actualClassName == expectedClassName)
            {
                return true;
            }

            // Handle lambda implementation classes
            if (actualClassName.find("$Lambda$") == 0)
            {
                // Extract the interface name from lambda class name
                size_t prefixEnd = actualClassName.find('$', 8); // Skip "$Lambda$"
                if (prefixEnd != std::string::npos)
                {
                    std::string lambdaInterfaceName = actualClassName.substr(8, prefixEnd - 8);

                    // Extract base interface name from expected class
                    std::string expectedBase = extractBaseClassName(expectedClassName);

                    // Check if the lambda implements the expected interface
                    if (lambdaInterfaceName == expectedBase)
                    {
                        return true;
                    }
                }
            }

            // Handle generic type compatibility
            std::string actualBase = extractBaseClassName(actualClassName);
            std::string expectedBase = extractBaseClassName(expectedClassName);

            // If base class names match, consider compatible for generic types
            if (actualBase == expectedBase)
            {
                return true;
            }

            return false;
        }

        std::string TypeValidator::resolveGenericClassName(
            const std::string& className,
            std::shared_ptr<EvaluationContext> context)
        {
            // First, try to resolve from context's generic type bindings (for generic methods/functions)
            const auto& contextBindings = context->getGenericTypeBindings();

            // Then, try to resolve from current instance's bindings (for generic classes)
            std::unordered_map<std::string, std::string> typeBindings = contextBindings;

            auto currentInstance = context->getCurrentInstance();
            if (currentInstance)
            {
                const auto& instanceBindings = currentInstance->getGenericTypeBindings();
                // Merge instance bindings into type bindings (context bindings take precedence)
                for (const auto& [key, value] : instanceBindings)
                {
                    if (typeBindings.find(key) == typeBindings.end())
                    {
                        typeBindings[key] = value;
                    }
                }
            }

            if (typeBindings.empty())
            {
                return className;
            }

            // Check if className is a simple type parameter (e.g., "T", "K", "V")
            auto it = typeBindings.find(className);
            if (it != typeBindings.end())
            {
                return it->second;
            }

            // Handle complex types with type parameters (e.g., "List<T>", "Map<K,V>")
            std::string resolvedClassName = className;
            for (const auto& [param, actualType] : typeBindings)
            {
                size_t pos = 0;
                while ((pos = resolvedClassName.find(param, pos)) != std::string::npos)
                {
                    // Check if this is a standalone type parameter (not part of another word)
                    bool isStandalone = (pos == 0 || !isalnum(resolvedClassName[pos - 1])) &&
                    (pos + param.length() >= resolvedClassName.length() ||
                        !isalnum(resolvedClassName[pos + param.length()]));

                    if (isStandalone)
                    {
                        resolvedClassName.replace(pos, param.length(), actualType);
                        pos += actualType.length();
                    }
                    else
                    {
                        pos++;
                    }
                }
            }

            return resolvedClassName;
        }

        std::string TypeValidator::extractBaseClassName(const std::string& className)
        {
            size_t pos = className.find('<');
            if (pos != std::string::npos)
            {
                return className.substr(0, pos);
            }
            return className;
        }

        bool TypeValidator::isSubclassCompatible(
            const std::string& actualClassName,
            const std::string& expectedClassName,
            std::shared_ptr<EvaluationContext> context)
        {
            if (!context)
            {
                return false;
            }

            // Extract base class names (remove generics)
            std::string actualBase = extractBaseClassName(actualClassName);
            std::string expectedBase = extractBaseClassName(expectedClassName);

            // Get the class definition for the actual class
            auto actualClassDef = context->getEnvironment()->findClass(actualBase);
            if (!actualClassDef)
            {
                return false;
            }

            // Check if actualClass is a subclass of expectedClass
            return actualClassDef->isSubclassOf(expectedBase);
        }
    } // namespace validation
} // namespace evaluator
