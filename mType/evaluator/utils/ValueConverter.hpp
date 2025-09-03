#pragma once
#include "../../value/ValueType.hpp"
#include <string>

namespace evaluator::utils
{
    using namespace value;
    
    /**
     * @brief Centralized type conversion utilities following DRY principle
     * Eliminates duplicate conversion logic across evaluator classes
     */
    class ValueConverter
    {
    public:
        /**
         * @brief Check if a value is truthy following mType language rules
         * @param value The value to check
         * @return true if value is truthy, false otherwise
         */
        static bool isTruthy(const Value& value);
        
        /**
         * @brief Convert value to string representation
         * @param value The value to convert
         * @return String representation of the value
         */
        static std::string toString(const Value& value);
        
        /**
         * @brief Convert value to float with type coercion
         * @param value The value to convert
         * @return Float representation of the value
         * @throws TypeException if conversion is not possible
         */
        static float toFloat(const Value& value);
        
        /**
         * @brief Convert value to integer with type coercion
         * @param value The value to convert
         * @return Integer representation of the value
         * @throws TypeException if conversion is not possible
         */
        static int toInt(const Value& value);
        
        /**
         * @brief Get the type of a value for error reporting
         * @param value The value to inspect
         * @return ValueType enum representing the type
         */
        static ValueType getValueType(const Value& value);
        
        /**
         * @brief Convert ValueType enum to string for error messages
         * @param type The ValueType to convert
         * @return String representation of the type
         */
        static std::string valueTypeToString(ValueType type);
        
        /**
         * @brief Compare two values for equality
         * @param left First value
         * @param right Second value
         * @return true if values are equal, false otherwise
         */
        static bool compareValues(const Value& left, const Value& right);
        
    private:
        // Static utility class - prevent instantiation
        ValueConverter() = default;
        ~ValueConverter() = default;
        ValueConverter(const ValueConverter&) = delete;
        ValueConverter& operator=(const ValueConverter&) = delete;
    };
}