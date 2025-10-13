#include "ParameterBinder.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../errors/ArgumentException.hpp"
#include "../../errors/TypeException.hpp"
#include "../../value/PromiseValue.hpp"
#include "../../value/NativeArray.hpp"
#include "ValueConverter.hpp"
#include <sstream>
#include <cctype>


namespace evaluator::utils
{
    using namespace value;
    using namespace environment;
    using namespace errors;
    using namespace runtimeTypes::global;

    void ParameterBinder::bindAndValidateParameters(
        const std::vector<std::pair<std::string, ParameterType>>& params,
        const std::vector<Value>& args,
        const std::string& functionName,
        std::shared_ptr<Environment> env,
        const SourceLocation& location)
    {
        // Call the version with empty generic bindings
        std::unordered_map<std::string, std::string> emptyBindings;
        bindAndValidateParameters(params, args, functionName, env, emptyBindings, location);
    }

    void ParameterBinder::bindAndValidateParameters(
        const std::vector<std::pair<std::string, ParameterType>>& params,
        const std::vector<Value>& args,
        const std::string& functionName,
        std::shared_ptr<Environment> env,
        const std::unordered_map<std::string, std::string>& genericBindings,
        const SourceLocation& location)
    {
        // Validate parameter count
        validateParameterCount(params.size(), args.size(), functionName);

        // Bind and validate each parameter
        for (size_t i = 0; i < params.size(); ++i)
        {
            const auto& param = params[i];
            const Value& arg = args[i];

            // Resolve generic type if needed
            ParameterType resolvedType = param.second;
            if (resolvedType.isClass())
            {
                std::string className = resolvedType.getClassName();
                auto it = genericBindings.find(className);
                if (it != genericBindings.end())
                {
                    // This is a generic type parameter - resolve it
                    resolvedType = ParameterType::forClass(it->second);
                }
            }

            // Enhanced validation with interface support
            validateParameterType(arg, resolvedType, param.first, functionName, env, location);

            // Create and bind parameter variable
            auto varDef = std::make_shared<VariableDefinition>(
                param.first,
                resolvedType.basicType,  // Use basic type for storage
                arg,
                false  // parameters are not final
            );

            env->declareVariable(param.first, varDef);
        }
    }

    // Backward compatibility method for old ValueType parameters
    void ParameterBinder::bindAndValidateParameters(
        const std::vector<std::pair<std::string, ValueType>>& params,
        const std::vector<Value>& args,
        const std::string& functionName,
        std::shared_ptr<Environment> env,
        const SourceLocation& location)
    {
        // Convert old format to new format and delegate
        auto newParams = ParameterType::fromValueTypeVector(params);
        bindAndValidateParameters(newParams, args, functionName, env, location);
    }

    void ParameterBinder::bindAndValidateParameters(
        std::shared_ptr<runtimeTypes::klass::MethodDefinition> method,
        const std::vector<Value>& args,
        const std::string& functionName,
        std::shared_ptr<Environment> env,
        const SourceLocation& location)
    {
        // Delegate to the overload with generic bindings, using method's type substitution map
        bindAndValidateParameters(method, args, functionName, env, method->getTypeSubstitutionMap(), location);
    }

    void ParameterBinder::bindAndValidateParameters(
        std::shared_ptr<runtimeTypes::klass::MethodDefinition> method,
        const std::vector<Value>& args,
        const std::string& functionName,
        std::shared_ptr<Environment> env,
        const std::unordered_map<std::string, std::string>& genericBindings,
        const SourceLocation& location)
    {
        auto params = method->getParameters();  // Now returns vector<pair<string, ParameterType>>

        // Validate parameter count
        validateParameterCount(params.size(), args.size(), functionName);

        // Bind and validate each parameter with enhanced interface type checking
        for (size_t i = 0; i < params.size(); ++i)
        {
            const auto& param = params[i];
            const Value& arg = args[i];

            // Resolve generic parameter type if needed
            ParameterType resolvedType = param.second;

            // If parameter has no className, check if it's a generic parameter from method's genericParameters
            if (!resolvedType.isClass() && !resolvedType.isInterface() && resolvedType.basicType == ValueType::OBJECT)
            {
                // Check genericParameters to get the actual type (e.g., "Array<T>" for T[] parameter)
                const auto& genericParams = method->getGenericParameters();
                if (i < genericParams.size())
                {
                    const auto& genericParam = genericParams[i];
                    auto genericType = genericParam.second;

                    if (genericType)
                    {
                        // Get the type string from the generic type (this includes array info)
                        std::string typeStr = genericType->toString();
                        // Create ParameterType from the generic type string
                        resolvedType = ParameterType::forClass(typeStr);
                    }
                }
            }

            if (resolvedType.isClass())
            {
                std::string className = resolvedType.getClassName();

                // Substitute all generic type parameters in className
                // Handle cases like "T", "Array<T>", "Array<Array<T>>", etc.
                std::string resolvedClassName = className;
                bool hadSubstitution = false;

                for (const auto& [typeParam, concreteType] : genericBindings)
                {
                    // Replace all occurrences of the type parameter
                    size_t pos = 0;
                    while ((pos = resolvedClassName.find(typeParam, pos)) != std::string::npos)
                    {
                        // Check if this is a standalone type parameter (not part of a larger identifier)
                        bool isStandalone = (pos == 0 || !std::isalnum(resolvedClassName[pos - 1])) &&
                                          (pos + typeParam.length() == resolvedClassName.length() ||
                                           !std::isalnum(resolvedClassName[pos + typeParam.length()]));

                        if (isStandalone)
                        {
                            resolvedClassName.replace(pos, typeParam.length(), concreteType);
                            pos += concreteType.length();
                            hadSubstitution = true;
                        }
                        else
                        {
                            pos += typeParam.length();
                        }
                    }
                }

                // Update resolved type with substituted class name
                if (hadSubstitution)
                {
                    resolvedType = ParameterType::forClass(resolvedClassName);
                }
                else if (resolvedClassName == className)
                {
                    // No substitution occurred - this might be a method-level generic parameter (like T in <T> method(T value))
                    // In this case, treat it as a plain OBJECT type for validation (no specific class requirement)
                    // This allows method-level generics to accept any object type
                    resolvedType = ParameterType(ValueType::OBJECT);
                }
            }

            // Enhanced validation with interface support
            validateParameterType(arg, resolvedType, param.first, functionName, env, location);

            // Create and bind parameter variable
            auto varDef = std::make_shared<VariableDefinition>(
                param.first,
                resolvedType.basicType,  // Use basic type for storage (object for interfaces/classes)
                arg,
                false  // parameters are not final
            );

            env->declareVariable(param.first, varDef);
        }
    }

    void ParameterBinder::validateParameterCount(
        size_t expected,
        size_t actual,
        const std::string& functionName)
    {
        if (expected != actual)
        {
            std::ostringstream oss;
            oss << "Function/method '" << functionName << "' expects " 
                << expected << " arguments, but " << actual << " were provided";
            throw ArgumentException(oss.str());
        }
    }

    bool ParameterBinder::isValidParameterConversion(ValueType from, ValueType to)
    {
        // Exact type match
        if (from == to) {
            return true;
        }

        // Allow null assignment to object types
        if (from == ValueType::NULL_TYPE && to == ValueType::OBJECT) {
            return true;
        }

        // Allow int to float conversion
        if (from == ValueType::INT && to == ValueType::FLOAT) {
            return true;
        }

        return false;
    }

    void ParameterBinder::validateParameterType(
        ValueType actual,
        ValueType expected,
        const std::string& paramName,
        const std::string& functionName,
        const SourceLocation& location)
    {
        if (!isValidParameterConversion(actual, expected))
        {
            std::string errorMsg = getTypeConversionErrorMessage(expected, actual, paramName, functionName);
            throw TypeException(errorMsg, location);
        }
    }

    std::string ParameterBinder::getTypeConversionErrorMessage(
        ValueType expected,
        ValueType actual,
        const std::string& paramName,
        const std::string& functionName)
    {
        std::ostringstream oss;
        oss << "Type mismatch for parameter '" << paramName 
            << "' in function/method '" << functionName << "': "
            << "expected " << ValueConverter::valueTypeToString(expected)
            << " but got " << ValueConverter::valueTypeToString(actual);
        return oss.str();
    }

    // New enhanced validation methods
    bool ParameterBinder::isValidParameterConversion(
        const Value& actualValue,
        const ParameterType& expectedType,
        std::shared_ptr<Environment> env)
    {
        // Handle non-object types with existing logic
        if (!expectedType.isInterface() && !expectedType.isClass()) {
            return checkBasicTypeConversion(actualValue, expectedType);
        }

        ValueType actualType = value::getValueType(actualValue); // Use global getValueType, not ValueConverter

        // Special handling for array types
        // Arrays are stored with className like "Array<int>" but actualType is ARRAY, not OBJECT
        if (expectedType.isClass() && actualType == ValueType::ARRAY) {
            std::string expectedClassName = expectedType.getClassName();

            // Check if expected type is an array type (starts with "Array<")
            if (expectedClassName.rfind("Array<", 0) == 0) {
                // Extract expected element type from "Array<ElementType>"
                // Need to handle nested generics like "Array<Array<int>>"
                size_t startPos = 6; // Length of "Array<"
                size_t endPos = startPos;
                int bracketDepth = 1; // We already have one '<' from "Array<"

                // Find matching '>' by counting bracket depth
                while (endPos < expectedClassName.length() && bracketDepth > 0) {
                    if (expectedClassName[endPos] == '<') {
                        bracketDepth++;
                    } else if (expectedClassName[endPos] == '>') {
                        bracketDepth--;
                    }
                    endPos++;
                }

                if (bracketDepth != 0) {
                    return false; // Malformed array type
                }

                // endPos is now one past the closing '>', so subtract 1
                std::string expectedElementType = expectedClassName.substr(startPos, endPos - startPos - 1);

                // Get actual array element type
                if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(actualValue)) {
                    auto nativeArray = std::get<std::shared_ptr<value::NativeArray>>(actualValue);
                    if (!nativeArray) {
                        return false;
                    }

                    ValueType actualElementType = nativeArray->getElementType();
                    std::string actualElementTypeName = nativeArray->getElementTypeName();

                    // Map ValueType to string for comparison
                    std::string actualElementTypeStr;
                    switch (actualElementType) {
                        case ValueType::INT: actualElementTypeStr = "int"; break;
                        case ValueType::FLOAT: actualElementTypeStr = "float"; break;
                        case ValueType::BOOL: actualElementTypeStr = "bool"; break;
                        case ValueType::STRING: actualElementTypeStr = "string"; break;
                        case ValueType::OBJECT:
                            actualElementTypeStr = actualElementTypeName.empty() ? "object" : actualElementTypeName;
                            break;
                        case ValueType::ARRAY:
                            // For nested arrays, use the element type name if available
                            actualElementTypeStr = actualElementTypeName.empty() ? "array" : actualElementTypeName;
                            break;
                        default: actualElementTypeStr = "unknown"; break;
                    }

                    // Compare element types
                    // For nested arrays, if actualElementTypeName is not set, we need to be more lenient
                    if (actualElementType == ValueType::ARRAY && actualElementTypeName.empty()) {
                        // Check if expected type is also an array type
                        if (expectedElementType.rfind("Array<", 0) == 0 || expectedElementType == "array") {
                            return true; // Accept nested arrays even without full type information
                        }
                    }
                    return expectedElementType == actualElementTypeStr;
                }

                // If not a NativeArray, reject
                return false;
            }
            return false;
        }

        // For interface/class parameters, actual value must be an object
        if (actualType != ValueType::OBJECT) {
            // Allow null for object types
            return actualType == ValueType::NULL_TYPE;
        }

        // Special case: PromiseValue matches Promise<T> class names
        if (std::holds_alternative<std::shared_ptr<value::PromiseValue>>(actualValue)) {
            if (expectedType.isClass()) {
                std::string expectedClassName = expectedType.getClassName();
                // Extract base class name (e.g., "Promise<Int>" -> "Promise")
                std::string baseExpectedClassName = expectedClassName;
                size_t anglePos = expectedClassName.find('<');
                if (anglePos != std::string::npos) {
                    baseExpectedClassName = expectedClassName.substr(0, anglePos);
                }
                // PromiseValue matches any Promise<T> type
                return baseExpectedClassName == "Promise";
            }
            return false;
        }

        // Get the object instance
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(actualValue)) {
            return false; // Not an object instance
        }

        auto objectInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(actualValue);
        if (!objectInstance) {
            return false; // Null object
        }

        // Check interface implementation
        if (expectedType.isInterface()) {
            return checkInterfaceImplementation(actualValue, expectedType, env);
        }

        // Check class type
        if (expectedType.isClass()) {
            return checkClassTypeMatch(actualValue, expectedType);
        }

        return false;
    }

    bool ParameterBinder::checkBasicTypeConversion(
        const Value& actualValue,
        const ParameterType& expectedType)
    {
        ValueType actualType = value::getValueType(actualValue); // Use global getValueType
        return isValidParameterConversion(actualType, expectedType.basicType);
    }

    bool ParameterBinder::checkInterfaceImplementation(
        const Value& actualValue,
        const ParameterType& expectedType,
        std::shared_ptr<Environment> env)
    {
        auto objectInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(actualValue);
        std::string interfaceName = expectedType.getInterfaceName();

        // Check if the object's class implements the required interface
        auto classDefinition = objectInstance->getClassDefinition();
        if (!classDefinition) {
            return false;
        }

        // Check if class implements the interface using registry for transitive checks
        auto interfaceRegistry = env->getInterfaceRegistry();
        if (interfaceRegistry) {
            return classDefinition->implementsInterface(interfaceName, interfaceRegistry);
        } else {
            // Fallback to direct interface checking only (no transitive inheritance)
            return classDefinition->implementsInterface(interfaceName);
        }
    }

    bool ParameterBinder::checkClassTypeMatch(
        const Value& actualValue,
        const ParameterType& expectedType)
    {
        auto objectInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(actualValue);
        std::string expectedClassName = expectedType.getClassName();

        auto classDefinition = objectInstance->getClassDefinition();
        if (!classDefinition) {
            return false;
        }

        // Extract base class name from expected type (strip generic arguments like <Int>)
        std::string baseExpectedClassName = expectedClassName;
        size_t anglePos = expectedClassName.find('<');
        if (anglePos != std::string::npos) {
            baseExpectedClassName = expectedClassName.substr(0, anglePos);
        }

        // Extract base class name from actual type as well
        std::string actualClassName = classDefinition->getName();
        std::string baseActualClassName = actualClassName;
        size_t actualAnglePos = actualClassName.find('<');
        if (actualAnglePos != std::string::npos) {
            baseActualClassName = actualClassName.substr(0, actualAnglePos);
        }

        // Check exact class match or inheritance using base class names
        return baseActualClassName == baseExpectedClassName ||
               classDefinition->isSubclassOf(baseExpectedClassName);
    }

    void ParameterBinder::validateParameterType(
        const Value& actualValue,
        const ParameterType& expectedType,
        const std::string& paramName,
        const std::string& functionName,
        std::shared_ptr<Environment> env,
        const SourceLocation& location)
    {
        ValueType actualType = value::getValueType(actualValue); // Use global getValueType
        bool isValid = isValidParameterConversion(actualValue, expectedType, env);

        if (!isValid)
        {
            // Enhanced error message for interface/class mismatches
            std::ostringstream oss;
            oss << "Parameter type mismatch for '" << paramName
                << "' in function/method '" << functionName << "': ";

            if (expectedType.isInterface()) {
                oss << "expected object implementing interface '" << expectedType.getInterfaceName() << "'";
            } else if (expectedType.isClass()) {
                oss << "expected object of class '" << expectedType.getClassName() << "'";
            } else {
                oss << "expected " << expectedType.toString();
            }

            oss << " but got ";

            if (actualType == ValueType::OBJECT &&
                std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(actualValue)) {
                auto objectInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(actualValue);
                if (objectInstance && objectInstance->getClassDefinition()) {
                    oss << "object of class '" << objectInstance->getClassDefinition()->getName() << "'";
                } else {
                    oss << "object";
                }
            } else {
                oss << ValueConverter::valueTypeToString(actualType);
            }

            throw TypeException(oss.str(), location);
        }
    }
}