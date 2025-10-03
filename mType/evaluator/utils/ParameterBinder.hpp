#pragma once

#include <vector>
#include <string>
#include <memory>
#include "../../value/ValueType.hpp"
#include "../../value/ParameterType.hpp"
#include "../../environment/Environment.hpp"
#include "../../errors/SourceLocation.hpp"

// Forward declaration
namespace runtimeTypes::klass {
    class MethodDefinition;
}

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
            const std::vector<std::pair<std::string, value::ParameterType>>& params,
            const std::vector<value::Value>& args,
            const std::string& functionName,
            std::shared_ptr<environment::Environment> env,
            const errors::SourceLocation& location = errors::SourceLocation{}
        );

        /**
         * Binds and validates parameters with generic type resolution
         * @param params Expected parameters (name, type pairs)
         * @param args Actual argument values
         * @param functionName Name of function/method for error messages
         * @param env Environment to bind variables in
         * @param genericBindings Map of generic type parameters to actual types (e.g., "T" -> "String")
         * @param location Source location for error reporting
         */
        static void bindAndValidateParameters(
            const std::vector<std::pair<std::string, value::ParameterType>>& params,
            const std::vector<value::Value>& args,
            const std::string& functionName,
            std::shared_ptr<environment::Environment> env,
            const std::unordered_map<std::string, std::string>& genericBindings,
            const errors::SourceLocation& location = errors::SourceLocation{}
        );

        /**
         * Backward compatibility: Binds and validates parameters with old ValueType format
         */
        static void bindAndValidateParameters(
            const std::vector<std::pair<std::string, value::ValueType>>& params,
            const std::vector<value::Value>& args,
            const std::string& functionName,
            std::shared_ptr<environment::Environment> env,
            const errors::SourceLocation& location = errors::SourceLocation{}
        );

        /**
         * NEW: Binds and validates parameters for generic method calls with runtime type resolution
         * @param method Method definition with generic type information
         * @param args Actual argument values
         * @param functionName Name of function/method for error messages
         * @param env Environment to bind variables in
         * @param location Source location for error reporting
         */
        static void bindAndValidateParameters(
            std::shared_ptr<runtimeTypes::klass::MethodDefinition> method,
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

        /**
         * Enhanced type validation that checks interface implementation
         */
        static bool isValidParameterConversion(
            const value::Value& actualValue,
            const value::ParameterType& expectedType,
            std::shared_ptr<environment::Environment> env
        );

    private:
        static void validateParameterType(
            value::ValueType expected,
            value::ValueType actual,
            const std::string& paramName,
            const std::string& functionName,
            const errors::SourceLocation& location
        );

        /**
         * Enhanced parameter type validation with interface support
         */
        static void validateParameterType(
            const value::Value& actualValue,
            const value::ParameterType& expectedType,
            const std::string& paramName,
            const std::string& functionName,
            std::shared_ptr<environment::Environment> env,
            const errors::SourceLocation& location
        );

        static std::string getTypeConversionErrorMessage(
            value::ValueType expected,
            value::ValueType actual,
            const std::string& paramName,
            const std::string& functionName
        );

        // Helper methods for isValidParameterConversion
        static bool checkBasicTypeConversion(
            const value::Value& actualValue,
            const value::ParameterType& expectedType
        );

        static bool checkInterfaceImplementation(
            const value::Value& actualValue,
            const value::ParameterType& expectedType,
            std::shared_ptr<environment::Environment> env
        );

        static bool checkClassTypeMatch(
            const value::Value& actualValue,
            const value::ParameterType& expectedType
        );
    };
}