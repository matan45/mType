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
        // Start with fresh substitution context
        const int MAX_SUBSTITUTION_DEPTH = 50; // Prevent extremely deep recursion
        SubstitutionContext context(MAX_SUBSTITUTION_DEPTH);

        return substituteInternal(substitutions, context);
    }

    bool GenericType::SubstitutionContext::enterSubstitution(const std::string& paramName)
    {
        // Check depth limit
        if (currentDepth >= maxDepth) {
            throw std::runtime_error("Maximum type substitution depth exceeded. Substitution chain: " + getChainString());
        }

        // Check for cycle - if parameter is already active in current chain
        if (activeParameters.find(paramName) != activeParameters.end()) {
            throw std::runtime_error("Circular generic type dependency detected. Substitution chain: " +
                getChainString() + " -> " + paramName);
        }

        // Track this substitution step
        substitutionChain.push_back(paramName);
        activeParameters.insert(paramName);
        parameterDepth[paramName] = currentDepth;
        currentDepth++;

        return true;
    }

    void GenericType::SubstitutionContext::exitSubstitution(const std::string& paramName)
    {
        // Remove from active set
        activeParameters.erase(paramName);

        // Remove from chain (should be at the end)
        if (!substitutionChain.empty() && substitutionChain.back() == paramName) {
            substitutionChain.pop_back();
        }

        currentDepth--;
    }

    std::string GenericType::SubstitutionContext::getChainString() const
    {
        if (substitutionChain.empty()) {
            return "(empty)";
        }

        std::string result = substitutionChain[0];
        for (size_t i = 1; i < substitutionChain.size(); ++i) {
            result += " -> " + substitutionChain[i];
        }
        return result;
    }

    std::shared_ptr<GenericType> GenericType::substituteInternal(
        const std::unordered_map<std::string, std::shared_ptr<GenericType>>& substitutions,
        SubstitutionContext& context) const
    {
        // If this is a generic parameter, check for substitution
        if (isGenericParameter()) {
            std::string paramName = getGenericName();

            auto it = substitutions.find(paramName);
            if (it != substitutions.end()) {
                // Enter substitution step with cycle detection
                context.enterSubstitution(paramName);

                try {
                    // Recursively substitute the replacement type
                    auto result = it->second->substituteInternal(substitutions, context);

                    // Exit substitution step
                    context.exitSubstitution(paramName);

                    return result;
                }
                catch (...) {
                    // Ensure we clean up context even on exception
                    context.exitSubstitution(paramName);
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
            substitutedArgs.push_back(arg->substituteInternal(substitutions, context));
        }

        return std::make_shared<GenericType>(getConcreteType(), substitutedArgs);
    }
}