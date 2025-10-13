#pragma once
#include "../../../value/ValueType.hpp"
#include <string>

namespace vm::runtime::utils
{
    /**
     * Utility class for type conversions
     * Provides static helper methods for converting between type representations
     */
    class TypeConverter
    {
    public:
        /**
         * Convert type name string to ValueType enum
         * @param typeName Type name (e.g., "int", "float", "string", "bool", "void")
         * @return Corresponding ValueType enum value
         */
        static value::ValueType stringToValueType(const std::string& typeName);

        /**
         * Extract base class name from generic type (e.g., "Box<Int>" -> "Box")
         * @param typeName Full type name potentially including generics
         * @return Base class name without generic parameters
         */
        static std::string extractBaseTypeName(const std::string& typeName);

        /**
         * Convert ValueType enum to string for error messages
         * @param type The ValueType to convert
         * @return String representation of the type
         */
        static std::string valueTypeToString(value::ValueType type);
    };
}
