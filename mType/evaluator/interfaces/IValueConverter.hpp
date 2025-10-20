#pragma once

#include "../../value/ValueType.hpp"
#include <string>

namespace evaluator
{
    namespace interfaces
    {
        using namespace value;

        /**
         * @brief Interface for value type conversions
         *
         * Provides abstraction for value conversion operations, enabling:
         * - Interface Segregation: Separates conversion concerns from evaluation
         * - Dependency Inversion: Clients depend on abstraction
         * - Testing: Mock implementations for unit testing
         * - Flexibility: Different conversion strategies
         *
         * Design Principles:
         * - Single Responsibility: Only handles type conversions
         * - Interface Segregation: Minimal, focused interface
         * - Open/Closed: New conversion strategies without modifying clients
         *
         * Separation Rationale:
         * - Type conversion is a utility concern, not core evaluation logic
         * - Handlers may need conversion without full evaluation capability
         * - Enables separate testing of conversion logic
         * - Allows for pluggable conversion strategies (strict, lenient, etc.)
         */
        class IValueConverter
        {
        public:
            virtual ~IValueConverter() = default;

            /**
             * @brief Convert a value to boolean (truthiness)
             * @param value The value to check
             * @return true if value is truthy
             *
             * Truthiness rules:
             * - Numbers: non-zero is true
             * - Strings: non-empty is true
             * - Objects: non-null is true
             * - Booleans: direct value
             * - null/monostate: false
             */
            virtual bool isTruthy(const Value& value) const = 0;

            /**
             * @brief Convert a value to string representation
             * @param value The value to convert
             * @return String representation
             *
             * Conversion rules:
             * - Numbers: decimal representation
             * - Strings: direct value
             * - Booleans: "true" or "false"
             * - Objects: toString() method or default representation
             * - null: "null"
             */
            virtual std::string toString(const Value& value) const = 0;

            /**
             * @brief Convert a value to float
             * @param value The value to convert
             * @return Float representation
             * @throws TypeConversionException if conversion not possible
             *
             * Conversion rules:
             * - Integers: direct conversion
             * - Floats: direct value
             * - Strings: parse as float
             * - Booleans: 1.0 for true, 0.0 for false
             * - Other types: throw exception
             */
            virtual float toFloat(const Value& value) const = 0;

            /**
             * @brief Convert a value to integer
             * @param value The value to convert
             * @return Integer representation
             * @throws TypeConversionException if conversion not possible
             *
             * Conversion rules:
             * - Integers: direct value
             * - Floats: truncate to integer
             * - Strings: parse as integer
             * - Booleans: 1 for true, 0 for false
             * - Other types: throw exception
             */
            virtual int toInt(const Value& value) const = 0;
        };
    }
}
