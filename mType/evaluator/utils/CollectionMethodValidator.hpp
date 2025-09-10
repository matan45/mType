#pragma once

#include "../../errors/TypeException.hpp"
#include "../../value/ValueType.hpp"
#include "ValueConverter.hpp"
#include <string>
#include <vector>

namespace evaluator::utils
{
    using namespace errors;
    using namespace value;

    /**
     * @brief Centralized validation for collection method calls
     * Eliminates duplicate validation code across all collection method handlers
     */
    class CollectionMethodValidator
    {
    public:
        /**
         * @brief Validate that a method takes no arguments
         * @param methodName Name of the method being called
         * @param args Arguments passed to the method
         * @throws TypeException if arguments are provided
         */
        static void validateNoArgs(const std::string& methodName, const std::vector<Value>& args)
        {
            if (!args.empty()) {
                throw TypeException(methodName + "() method takes no arguments");
            }
        }

        /**
         * @brief Validate exact argument count
         * @param methodName Name of the method being called
         * @param args Arguments passed to the method
         * @param expectedCount Expected number of arguments
         * @throws TypeException if argument count doesn't match
         */
        static void validateArgCount(const std::string& methodName, 
                                   const std::vector<Value>& args, 
                                   size_t expectedCount)
        {
            if (args.size() != expectedCount) {
                throw TypeException(methodName + "() method takes exactly " + 
                                  std::to_string(expectedCount) + " argument" +
                                  (expectedCount == 1 ? "" : "s"));
            }
        }

        /**
         * @brief Extract and validate integer index from Value
         * @param value The value to extract from
         * @param context Context string for error messages
         * @return Valid non-negative integer index
         * @throws TypeException if not an integer or negative
         */
        static int extractIntIndex(const Value& value, const std::string& context)
        {
            if (!std::holds_alternative<int>(value)) {
                throw TypeException(context + " must be an integer");
            }
            
            int index = std::get<int>(value);
            if (index < 0) {
                throw TypeException(context + " cannot be negative");
            }
            
            return index;
        }

        /**
         * @brief Convert Value to string for use as map key
         * @param value The value to convert
         * @return String representation
         */
        static std::string extractStringKey(const Value& value)
        {
            return ValueConverter::toString(value);
        }

        /**
         * @brief Validate that a value is not null
         * @param value The value to check
         * @param context Context for error message
         * @throws TypeException if value is null
         */
        static void validateNotNull(const Value& value, const std::string& context)
        {
            if (std::holds_alternative<nullptr_t>(value)) {
                throw TypeException("Cannot " + context + " with null value");
            }
        }
    };
}