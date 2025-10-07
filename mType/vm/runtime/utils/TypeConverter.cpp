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
}
