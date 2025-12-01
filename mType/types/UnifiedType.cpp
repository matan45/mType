#include "UnifiedType.hpp"
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace types
{
    // ============== TypeConstraint Implementation ==============

    bool TypeConstraint::operator==(const TypeConstraint& other) const
    {
        if (kind != other.kind) return false;
        if (!boundType && !other.boundType) return true;
        if (!boundType || !other.boundType) return false;
        return boundType->equals(other.boundType);
    }

    std::string TypeConstraint::toString() const
    {
        std::string result = (kind == Kind::Extends) ? "extends " : "implements ";
        result += boundType ? boundType->toString() : "<null>";
        return result;
    }

    // ============== UnifiedType Constructor ==============

    UnifiedType::UnifiedType(TypeKind k, std::string n,
                             std::vector<UnifiedTypePtr> args,
                             std::vector<TypeConstraint> cons,
                             bool isNullable,
                             errors::SourceLocation loc)
        : kind(k)
        , name(std::move(n))
        , typeArguments(std::move(args))
        , constraints(std::move(cons))
        , nullable(isNullable)
        , location(std::move(loc))
    {
    }

    // ============== Factory Methods ==============

    UnifiedTypePtr UnifiedType::primitive(value::ValueType vt)
    {
        std::string name = valueTypeToString(vt);
        return std::shared_ptr<UnifiedType>(new UnifiedType(
            TypeKind::Primitive, name, {}, {}, false, errors::SourceLocation()));
    }

    UnifiedTypePtr UnifiedType::classType(const std::string& name,
                                          std::vector<UnifiedTypePtr> args)
    {
        return std::shared_ptr<UnifiedType>(new UnifiedType(
            TypeKind::Class, name, std::move(args), {}, false, errors::SourceLocation()));
    }

    UnifiedTypePtr UnifiedType::interfaceType(const std::string& name,
                                              std::vector<UnifiedTypePtr> args)
    {
        return std::shared_ptr<UnifiedType>(new UnifiedType(
            TypeKind::Interface, name, std::move(args), {}, false, errors::SourceLocation()));
    }

    UnifiedTypePtr UnifiedType::genericParam(const std::string& name,
                                              std::vector<TypeConstraint> constraints)
    {
        return std::shared_ptr<UnifiedType>(new UnifiedType(
            TypeKind::GenericParameter, name, {}, std::move(constraints), false, errors::SourceLocation()));
    }

    UnifiedTypePtr UnifiedType::arrayOf(UnifiedTypePtr elementType)
    {
        std::vector<UnifiedTypePtr> args;
        if (elementType)
        {
            args.push_back(std::move(elementType));
        }
        return std::shared_ptr<UnifiedType>(new UnifiedType(
            TypeKind::Array, "Array", std::move(args), {}, false, errors::SourceLocation()));
    }

    UnifiedTypePtr UnifiedType::voidType()
    {
        return std::shared_ptr<UnifiedType>(new UnifiedType(
            TypeKind::Void, "void", {}, {}, false, errors::SourceLocation()));
    }

    UnifiedTypePtr UnifiedType::nullType()
    {
        return std::shared_ptr<UnifiedType>(new UnifiedType(
            TypeKind::Null, "null", {}, {}, true, errors::SourceLocation()));
    }

    UnifiedTypePtr UnifiedType::lambdaType(const std::string& name)
    {
        return std::shared_ptr<UnifiedType>(new UnifiedType(
            TypeKind::Lambda, name.empty() ? "Lambda" : name, {}, {}, false, errors::SourceLocation()));
    }

    // ============== Type Queries ==============

    bool UnifiedType::containsGenericParameters() const
    {
        if (isGenericParameter())
        {
            return true;
        }

        for (const auto& arg : typeArguments)
        {
            if (arg && arg->containsGenericParameters())
            {
                return true;
            }
        }

        return false;
    }

    UnifiedTypePtr UnifiedType::getElementType() const
    {
        if (kind != TypeKind::Array)
        {
            throw std::runtime_error("getElementType() called on non-array type: " + toString());
        }

        if (typeArguments.empty())
        {
            throw std::runtime_error("Array type has no element type");
        }

        return typeArguments[0];
    }

    value::ValueType UnifiedType::toValueType() const
    {
        switch (kind)
        {
            case TypeKind::Primitive:
                if (name == "int" || name == "Int") return value::ValueType::INT;
                if (name == "float" || name == "Float") return value::ValueType::FLOAT;
                if (name == "bool" || name == "Bool") return value::ValueType::BOOL;
                if (name == "string" || name == "String") return value::ValueType::STRING;
                return value::ValueType::OBJECT;

            case TypeKind::Void:
                return value::ValueType::VOID;

            case TypeKind::Null:
                return value::ValueType::NULL_TYPE;

            case TypeKind::Array:
                return value::ValueType::ARRAY;

            case TypeKind::Lambda:
                return value::ValueType::LAMBDA;

            case TypeKind::Class:
            case TypeKind::Interface:
            case TypeKind::GenericParameter:
            default:
                return value::ValueType::OBJECT;
        }
    }

    // ============== Type Operations ==============

    UnifiedTypePtr UnifiedType::substitute(const TypeSubstitutionMap& substitutions) const
    {
        if (substitutions.empty())
        {
            return shared_from_this();
        }

        std::unordered_set<std::string> visited;
        return substituteInternal(substitutions, visited, 0);
    }

    UnifiedTypePtr UnifiedType::substituteInternal(
        const TypeSubstitutionMap& substitutions,
        std::unordered_set<std::string>& visited,
        int depth) const
    {
        if (depth > MAX_SUBSTITUTION_DEPTH)
        {
            throw std::runtime_error("Maximum type substitution depth exceeded. "
                                     "Possible circular type dependency.");
        }

        // If this is a generic parameter, look for substitution
        if (isGenericParameter())
        {
            auto it = substitutions.find(name);
            if (it != substitutions.end() && it->second)
            {
                // Check for circular substitution
                if (visited.count(name) > 0)
                {
                    throw std::runtime_error("Circular type substitution detected for: " + name);
                }

                visited.insert(name);

                // Recursively substitute the replacement type
                // (in case it also contains generic parameters)
                auto result = it->second->substituteInternal(substitutions, visited, depth + 1);

                visited.erase(name);
                return result;
            }

            // No substitution found, return self
            return shared_from_this();
        }

        // For non-generic types, substitute type arguments
        if (typeArguments.empty())
        {
            return shared_from_this();
        }

        std::vector<UnifiedTypePtr> substitutedArgs;
        substitutedArgs.reserve(typeArguments.size());
        bool anyChanged = false;

        for (const auto& arg : typeArguments)
        {
            if (!arg)
            {
                substitutedArgs.push_back(nullptr);
                continue;
            }

            auto substitutedArg = arg->substituteInternal(substitutions, visited, depth + 1);
            if (substitutedArg.get() != arg.get())
            {
                anyChanged = true;
            }
            substitutedArgs.push_back(std::move(substitutedArg));
        }

        if (!anyChanged)
        {
            return shared_from_this();
        }

        // Create new type with substituted arguments
        return std::shared_ptr<UnifiedType>(new UnifiedType(
            kind, name, std::move(substitutedArgs), constraints, nullable, location));
    }

    UnifiedTypePtr UnifiedType::makeNullable() const
    {
        if (nullable)
        {
            return shared_from_this();
        }

        return std::shared_ptr<UnifiedType>(new UnifiedType(
            kind, name, typeArguments, constraints, true, location));
    }

    UnifiedTypePtr UnifiedType::makeNonNullable() const
    {
        if (!nullable)
        {
            return shared_from_this();
        }

        return std::shared_ptr<UnifiedType>(new UnifiedType(
            kind, name, typeArguments, constraints, false, location));
    }

    UnifiedTypePtr UnifiedType::withLocation(const errors::SourceLocation& loc) const
    {
        return std::shared_ptr<UnifiedType>(new UnifiedType(
            kind, name, typeArguments, constraints, nullable, loc));
    }

    // ============== Comparison & Identity ==============

    bool UnifiedType::equals(const UnifiedTypePtr& other) const
    {
        if (!other) return false;
        return *this == *other;
    }

    bool UnifiedType::operator==(const UnifiedType& other) const
    {
        if (kind != other.kind) return false;
        if (name != other.name) return false;
        if (nullable != other.nullable) return false;
        if (typeArguments.size() != other.typeArguments.size()) return false;

        for (size_t i = 0; i < typeArguments.size(); ++i)
        {
            const auto& a = typeArguments[i];
            const auto& b = other.typeArguments[i];

            if (!a && !b) continue;
            if (!a || !b) return false;
            if (!a->equals(b)) return false;
        }

        // Note: Constraints are not compared for structural equality
        // Two types with same name/args but different constraints are considered equal
        // Constraint checking is a separate validation step

        return true;
    }

    size_t UnifiedType::hash() const
    {
        size_t h = std::hash<int>{}(static_cast<int>(kind));
        h ^= std::hash<std::string>{}(name) << 1;
        h ^= std::hash<bool>{}(nullable) << 2;

        for (const auto& arg : typeArguments)
        {
            if (arg)
            {
                h ^= arg->hash() << 3;
            }
        }

        return h;
    }

    std::string UnifiedType::toCanonicalString() const
    {
        std::ostringstream oss;

        switch (kind)
        {
            case TypeKind::Array:
                if (!typeArguments.empty() && typeArguments[0])
                {
                    oss << typeArguments[0]->toCanonicalString() << "[]";
                }
                else
                {
                    oss << "Array";
                }
                break;

            case TypeKind::GenericParameter:
                oss << name;
                break;

            default:
                oss << name;
                if (!typeArguments.empty())
                {
                    oss << "<";
                    for (size_t i = 0; i < typeArguments.size(); ++i)
                    {
                        if (i > 0) oss << ",";
                        if (typeArguments[i])
                        {
                            oss << typeArguments[i]->toCanonicalString();
                        }
                    }
                    oss << ">";
                }
                break;
        }

        if (nullable && kind != TypeKind::Null)
        {
            oss << "?";
        }

        return oss.str();
    }

    std::string UnifiedType::toString() const
    {
        std::ostringstream oss;

        switch (kind)
        {
            case TypeKind::Array:
                if (!typeArguments.empty() && typeArguments[0])
                {
                    oss << typeArguments[0]->toString() << "[]";
                }
                else
                {
                    oss << "Array";
                }
                break;

            case TypeKind::GenericParameter:
                oss << name;
                if (!constraints.empty())
                {
                    oss << " ";
                    for (size_t i = 0; i < constraints.size(); ++i)
                    {
                        if (i > 0) oss << " & ";
                        oss << constraints[i].toString();
                    }
                }
                break;

            default:
                oss << name;
                if (!typeArguments.empty())
                {
                    oss << "<";
                    for (size_t i = 0; i < typeArguments.size(); ++i)
                    {
                        if (i > 0) oss << ", ";
                        if (typeArguments[i])
                        {
                            oss << typeArguments[i]->toString();
                        }
                    }
                    oss << ">";
                }
                break;
        }

        if (nullable && kind != TypeKind::Null)
        {
            oss << "?";
        }

        return oss.str();
    }

    // ============== Helper Methods ==============

    std::string UnifiedType::valueTypeToString(value::ValueType vt)
    {
        switch (vt)
        {
            case value::ValueType::INT: return "int";
            case value::ValueType::FLOAT: return "float";
            case value::ValueType::BOOL: return "bool";
            case value::ValueType::STRING: return "string";
            case value::ValueType::VOID: return "void";
            case value::ValueType::OBJECT: return "Object";
            case value::ValueType::ARRAY: return "Array";
            case value::ValueType::LAMBDA: return "Lambda";
            case value::ValueType::NULL_TYPE: return "null";
            default: return "unknown";
        }
    }

    std::string UnifiedType::kindToString(TypeKind k)
    {
        switch (k)
        {
            case TypeKind::Primitive: return "Primitive";
            case TypeKind::Class: return "Class";
            case TypeKind::Interface: return "Interface";
            case TypeKind::GenericParameter: return "GenericParameter";
            case TypeKind::Array: return "Array";
            case TypeKind::Void: return "Void";
            case TypeKind::Null: return "Null";
            case TypeKind::Lambda: return "Lambda";
            default: return "Unknown";
        }
    }
}
