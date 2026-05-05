#include "GenericType.hpp"
#include <cstddef>
#include "../errors/TypeException.hpp"
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace ast
{
    value::ValueType GenericType::getConcreteType() const
    {
        if (isGenericParameter())
        {
            throw errors::TypeException("Cannot get concrete type from generic parameter: " +
                std::get<std::string>(baseType));
        }
        return std::get<value::ValueType>(baseType);
    }

    std::string GenericType::getGenericName() const
    {
        if (!isGenericParameter())
        {
            throw errors::TypeException("Cannot get generic name from concrete type");
        }
        return std::get<std::string>(baseType);
    }

    std::string GenericType::getBaseTypeName() const
    {
        if (isGenericParameter())
        {
            return std::get<std::string>(baseType);
        }

        // Convert ValueType to string
        value::ValueType type = std::get<value::ValueType>(baseType);
        switch (type)
        {
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
        std::string baseName = getBaseTypeName();

        // Special handling for Array types: Array<T> should be rendered as T[]
        if (baseName == "Array" && typeArguments.size() == 1)
        {
            // Check if the element type is also an array (for multi-dimensional arrays)
            std::string elementTypeStr = typeArguments[0]->toString();
            oss << elementTypeStr << "[]";
            if (nullable) oss << "?";
            return oss.str();
        }

        oss << baseName;

        if (isParameterized())
        {
            oss << "<";
            for (size_t i = 0; i < typeArguments.size(); ++i)
            {
                if (i > 0) oss << ", ";
                oss << typeArguments[i]->toString();
            }
            oss << ">";
        }

        if (nullable) oss << "?";

        return oss.str();
    }

    bool GenericType::equals(const GenericType& other) const
    {
        // Check if base types match — handle cross-variant comparison
        // e.g., GenericType("int") should equal GenericType(ValueType::INT)
        if (baseType != other.baseType)
        {
            // If both hold the same variant alternative, they're different
            if (baseType.index() == other.baseType.index())
            {
                return false;
            }

            // Cross-variant: compare via normalized base type name
            if (getBaseTypeName() != other.getBaseTypeName())
            {
                return false;
            }
        }

        // Check nullable match
        if (nullable != other.nullable)
        {
            return false;
        }

        // Check if type argument counts match
        if (typeArguments.size() != other.typeArguments.size())
        {
            return false;
        }

        // Check if all type arguments match
        for (size_t i = 0; i < typeArguments.size(); ++i)
        {
            if (!typeArguments[i]->equals(*other.typeArguments[i]))
            {
                return false;
            }
        }

        return true;
    }

    std::shared_ptr<GenericType> GenericType::substitute(
        const std::unordered_map<std::string, std::shared_ptr<GenericType>>& substitutions) const
    {
        // Start with enhanced substitution context
        circularDependency::CircularDependencyConfig config;
        config.maxGenericDepth = 50; // Configurable depth limit
        config.enableEarlyDetection = true; // Enable pattern detection
        config.enablePerformanceMetrics = false; // Only enable for profiling

        SubstitutionContext context(config);
        context.currentLocation = "generic type substitution";

        return substituteInternal(substitutions, context);
    }


    std::shared_ptr<GenericType> GenericType::substituteInternal(
        const std::unordered_map<std::string, std::shared_ptr<GenericType>>& substitutions,
        SubstitutionContext& context) const
    {
        // If this is a generic parameter, check for substitution
        if (isGenericParameter())
        {
            std::string paramName = getGenericName();

            auto it = substitutions.find(paramName);
            if (it != substitutions.end())
            {
                // Enter substitution step with cycle detection
                context.enterSubstitution(paramName);

                // RAII guard ensures exitSubstitution is called on all exit paths
                struct SubstitutionGuard
                {
                    SubstitutionContext& ctx;
                    const std::string& name;
                    SubstitutionGuard(SubstitutionContext& c, const std::string& n) : ctx(c), name(n) {}
                    ~SubstitutionGuard() { ctx.exitSubstitution(name); }
                } guard(context, paramName);

                return it->second->substituteInternal(substitutions, context);
            }

            // No substitution found, return copy of this parameter
            return std::make_shared<GenericType>(*this);
        }

        // For concrete types, create new instance with substituted type arguments
        std::vector<std::shared_ptr<GenericType>> substitutedArgs;
        substitutedArgs.reserve(typeArguments.size()); // Optimize memory allocation

        for (const auto& arg : typeArguments)
        {
            substitutedArgs.push_back(arg->substituteInternal(substitutions, context));
        }

        auto result = std::make_shared<GenericType>(getConcreteType(), substitutedArgs);
        result->setNullable(nullable);
        return result;
    }
}
