#include "TypeConverter.hpp"

namespace vm::runtime::utils
{
    value::ValueType TypeConverter::stringToValueType(const std::string& typeName) {
        if (typeName == "int") return value::ValueType::INT;
        if (typeName == "float") return value::ValueType::FLOAT;
        if (typeName == "bool") return value::ValueType::BOOL;
        if (typeName == "string") return value::ValueType::STRING;
        if (typeName == "void") return value::ValueType::VOID;
        // For any object/class type
        return value::ValueType::OBJECT;
    }

    std::string TypeConverter::extractBaseTypeName(const std::string& typeName) {
        size_t genericStart = typeName.find('<');
        if (genericStart != std::string::npos) {
            return typeName.substr(0, genericStart);
        }
        return typeName;
    }

    std::string TypeConverter::valueTypeToString(value::ValueType type) {
        switch (type) {
            case value::ValueType::INT: return "int";
            case value::ValueType::FLOAT: return "float";
            case value::ValueType::STRING: return "string";
            case value::ValueType::BOOL: return "bool";
            case value::ValueType::OBJECT: return "object";
            case value::ValueType::NULL_TYPE: return "null";
            case value::ValueType::VOID: return "void";
            default: return "unknown";
        }
    }
}
