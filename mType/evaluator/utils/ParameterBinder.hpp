#pragma once

#include <vector>
#include <string>
#include <memory>
#include "../../value/ValueType.hpp"
#include "../../environment/Environment.hpp"
#include "../../errors/SourceLocation.hpp"

namespace evaluator::utils
{
    class ParameterBinder
    {
    public:
        /**
         * Binds and validates parameters for function/method calls
         * @param params Expected parameters (name, type pairs)
         * @param args Actual argument values
         * @param functionName Name of function/method for error messages
         * @param env Environment to bind variables in
         * @param location Source location for error reporting
         */
        static void bindAndValidateParameters(
            const std::vector<std::pair<std::string, value::ValueType>>& params,
            const std::vector<value::Value>& args,
            const std::string& functionName,
            std::shared_ptr<environment::Environment> env,
            const errors::SourceLocation& location = errors::SourceLocation{}
        );

        /**
         * Validates parameter count without binding
         */
        static void validateParameterCount(
            size_t expected,
            size_t actual,
            const std::string& functionName
        );

        /**
         * Checks if a type conversion is valid for parameter passing
         */
        static bool isValidParameterConversion(value::ValueType from, value::ValueType to);

    private:
        static void validateParameterType(
            value::ValueType expected,
            value::ValueType actual,
            const std::string& paramName,
            const std::string& functionName,
            const errors::SourceLocation& location
        );

        static std::string getTypeConversionErrorMessage(
            value::ValueType expected,
            value::ValueType actual,
            const std::string& paramName,
            const std::string& functionName
        );
    };
}