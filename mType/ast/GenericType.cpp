#include "GenericType.hpp"
#include <stdexcept>
#include <sstream>
#include <unordered_map>

namespace ast
{
    value::ValueType GenericType::getConcreteType() const
    {
        if (isGenericParameter()) {
            throw std::runtime_error("Cannot get concrete type from generic parameter: " + std::get<std::string>(baseType));
        }
        return std::get<value::ValueType>(baseType);
    }

    std::string GenericType::getGenericName() const
    {
        if (!isGenericParameter()) {
            throw std::runtime_error("Cannot get generic name from concrete type");
        }
        return std::get<std::string>(baseType);
    }

    std::string GenericType::getBaseTypeName() const
    {
        if (isGenericParameter()) {
            return std::get<std::string>(baseType);
        }

        // Convert ValueType to string
        value::ValueType type = std::get<value::ValueType>(baseType);
        switch (type) {
            case value::ValueType::INT: return "int";
            case value::ValueType::FLOAT: return "float";
            case value::ValueType::BOOL: return "bool";
            case value::ValueType::STRING: return "string";
            case value::ValueType::VOID: return "void";
            case value::ValueType::OBJECT: return "object";
            case value::ValueType::NULL_TYPE: return "null";
            // Collection types removed - now implemented in mType
            default: return "unknown";
        }
    }

    std::string GenericType::toString() const
    {
        std::ostringstream oss;
        oss << getBaseTypeName();

        if (isParameterized()) {
            oss << "<";
            for (size_t i = 0; i < typeArguments.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << typeArguments[i]->toString();
            }
            oss << ">";
        }

        return oss.str();
    }

    bool GenericType::equals(const GenericType& other) const
    {
        // Check if base types match
        if (baseType != other.baseType) {
            return false;
        }

        // Check if type argument counts match
        if (typeArguments.size() != other.typeArguments.size()) {
            return false;
        }

        // Check if all type arguments match
        for (size_t i = 0; i < typeArguments.size(); ++i) {
            if (!typeArguments[i]->equals(*other.typeArguments[i])) {
                return false;
            }
        }

        return true;
    }

    std::shared_ptr<GenericType> GenericType::substitute(
        const std::unordered_map<std::string, std::shared_ptr<GenericType>>& substitutions) const
    {
        // If this is a generic parameter, check for substitution
        if (isGenericParameter()) {
            std::string paramName = getGenericName();
            auto it = substitutions.find(paramName);
            if (it != substitutions.end()) {
                return std::make_shared<GenericType>(*it->second);
            }
            // No substitution found, return copy of this parameter
            return std::make_shared<GenericType>(*this);
        }

        // For concrete types, create new instance with substituted type arguments
        std::vector<std::shared_ptr<GenericType>> substitutedArgs;
        for (const auto& arg : typeArguments) {
            substitutedArgs.push_back(arg->substitute(substitutions));
        }

        return std::make_shared<GenericType>(getConcreteType(), substitutedArgs);
    }
}