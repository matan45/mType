#include "ParameterBinder.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../errors/ArgumentException.hpp"
#include "../../errors/TypeException.hpp"
#include "ValueConverter.hpp"
#include <sstream>

namespace evaluator::utils
{
    using namespace value;
    using namespace environment;
    using namespace errors;
    using namespace runtimeTypes::global;

    void ParameterBinder::bindAndValidateParameters(
        const std::vector<std::pair<std::string, ValueType>>& params,
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
            
            // Get actual argument type
            ValueType actualType = ValueConverter::getValueType(arg);
            ValueType expectedType = param.second;
            
            // Validate type compatibility
            validateParameterType(actualType, expectedType, param.first, functionName, location);
            
            // Create and bind parameter variable
            auto varDef = std::make_shared<VariableDefinition>(
                param.first, 
                expectedType, 
                arg, 
                false  // parameters are not final
            );
            
            env->declareVariable(param.first, varDef);
        }
    }

    void ParameterBinder::bindAndValidateParameters(
        std::shared_ptr<runtimeTypes::klass::MethodDefinition> method,
        const std::vector<Value>& args,
        const std::string& functionName,
        std::shared_ptr<Environment> env,
        const SourceLocation& location)
    {
        auto params = method->getParameters();

        // Validate parameter count
        validateParameterCount(params.size(), args.size(), functionName);

        // Bind and validate each parameter with runtime type resolution
        for (size_t i = 0; i < params.size(); ++i)
        {
            const auto& param = params[i];
            const Value& arg = args[i];

            // Get actual argument type
            ValueType actualType = ValueConverter::getValueType(arg);

            // Get expected type with runtime resolution for generics
            ValueType expectedType = method->resolveParameterType(i);

            // Validate type compatibility
            validateParameterType(actualType, expectedType, param.first, functionName, location);

            // Create and bind parameter variable
            // For object types, preserve the actual object type to maintain method bindings
            ValueType typeToUse = actualType;
            if (actualType == ValueType::OBJECT && expectedType != ValueType::OBJECT) {
                // If we have a concrete object but expect a generic type, keep the object type
                typeToUse = actualType;
            } else if (expectedType != ValueType::OBJECT) {
                // For non-object types, use the expected type
                typeToUse = expectedType;
            }

            auto varDef = std::make_shared<VariableDefinition>(
                param.first,
                typeToUse,  // Use appropriate type to preserve object method bindings
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
}