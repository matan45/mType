#include "GenericType.hpp"
#include <stdexcept>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

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
        // Start with empty visited set and reasonable max depth
        std::unordered_set<std::string> visitedTypes;
        const int MAX_SUBSTITUTION_DEPTH = 50; // Prevent extremely deep recursion

        return substituteInternal(substitutions, visitedTypes, MAX_SUBSTITUTION_DEPTH, 0);
    }

    std::shared_ptr<GenericType> GenericType::substituteInternal(
        const std::unordered_map<std::string, std::shared_ptr<GenericType>>& substitutions,
        std::unordered_set<std::string>& visitedTypes,
        int maxDepth,
        int currentDepth) const
    {
        // Prevent excessive recursion depth
        if (currentDepth >= maxDepth) {
            throw std::runtime_error("Maximum type substitution depth exceeded. Possible circular generic type dependency.");
        }

        // If this is a generic parameter, check for substitution
        if (isGenericParameter()) {
            std::string paramName = getGenericName();

            // Check for circular reference before substitution
            if (visitedTypes.find(paramName) != visitedTypes.end()) {
                throw std::runtime_error("Circular generic type dependency detected: " + paramName +
                    " references itself directly or indirectly during substitution.");
            }

            auto it = substitutions.find(paramName);
            if (it != substitutions.end()) {
                // Add this parameter to visited set to detect cycles
                visitedTypes.insert(paramName);

                try {
                    // Recursively substitute the replacement type
                    auto result = it->second->substituteInternal(substitutions, visitedTypes, maxDepth, currentDepth + 1);

                    // Remove from visited set when done
                    visitedTypes.erase(paramName);

                    return result;
                }
                catch (...) {
                    // Ensure we clean up visited set even on exception
                    visitedTypes.erase(paramName);
                    throw;
                }
            }

            // No substitution found, return copy of this parameter
            return std::make_shared<GenericType>(*this);
        }

        // For concrete types, create new instance with substituted type arguments
        std::vector<std::shared_ptr<GenericType>> substitutedArgs;
        substitutedArgs.reserve(typeArguments.size()); // Optimize memory allocation

        for (const auto& arg : typeArguments) {
            substitutedArgs.push_back(arg->substituteInternal(substitutions, visitedTypes, maxDepth, currentDepth + 1));
        }

        return std::make_shared<GenericType>(getConcreteType(), substitutedArgs);
    }
}