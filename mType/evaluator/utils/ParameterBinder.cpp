#include "ParameterBinder.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../errors/ArgumentException.hpp"
#include "../../errors/TypeException.hpp"
#include "ValueConverter.hpp"
#include <sstream>
#include <iostream>

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
        // Validate parameter count
        validateParameterCount(params.size(), args.size(), functionName);

        // Bind and validate each parameter
        for (size_t i = 0; i < params.size(); ++i)
        {
            const auto& param = params[i];
            const Value& arg = args[i];

            // Enhanced validation with interface support
            validateParameterType(arg, param.second, param.first, functionName, env, location);

            // Create and bind parameter variable
            auto varDef = std::make_shared<VariableDefinition>(
                param.first,
                param.second.basicType,  // Use basic type for storage
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
        auto params = method->getParameters();  // Now returns vector<pair<string, ParameterType>>

        // Validate parameter count
        validateParameterCount(params.size(), args.size(), functionName);

        // Bind and validate each parameter with enhanced interface type checking
        for (size_t i = 0; i < params.size(); ++i)
        {
            const auto& param = params[i];
            const Value& arg = args[i];

            // Enhanced validation with interface support
            validateParameterType(arg, param.second, param.first, functionName, env, location);

            // Create and bind parameter variable
            auto varDef = std::make_shared<VariableDefinition>(
                param.first,
                param.second.basicType,  // Use basic type for storage (object for interfaces/classes)
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
        ValueType actualType = ValueConverter::getValueType(actualValue);

        // Handle non-object types with existing logic
        if (!expectedType.isInterface() && !expectedType.isClass()) {
            return isValidParameterConversion(actualType, expectedType.basicType);
        }

        // For interface/class parameters, actual value must be an object
        if (actualType != ValueType::OBJECT) {
            // Allow null for object types
            return actualType == ValueType::NULL_TYPE;
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
                return classDefinition->implementsInterface(interfaceName);
            }
        }

        // Check class type (exact class or inheritance)
        if (expectedType.isClass()) {
            std::string expectedClassName = expectedType.getClassName();

            auto classDefinition = objectInstance->getClassDefinition();
            if (!classDefinition) {
                return false;
            }

            // Check exact class match or inheritance
            return classDefinition->getName() == expectedClassName ||
                   classDefinition->isSubclassOf(expectedClassName);
        }

        return false;
    }

    void ParameterBinder::validateParameterType(
        const Value& actualValue,
        const ParameterType& expectedType,
        const std::string& paramName,
        const std::string& functionName,
        std::shared_ptr<Environment> env,
        const SourceLocation& location)
    {
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

            ValueType actualType = ValueConverter::getValueType(actualValue);
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