#pragma once

#include "../value/ValueType.hpp"
#include <cstddef>
#include <cstdint>
#include "../errors/SourceLocation.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace types
{
    class UnifiedType;
    struct TypeConstraint;

    using UnifiedTypePtr = std::shared_ptr<const UnifiedType>;
    using TypeSubstitutionMap = std::unordered_map<std::string, UnifiedTypePtr>;

    /**
     * Represents the kind of type in the unified type system.
     */
    enum class TypeKind : uint8_t
    {
        Primitive,        // int, float, bool, string
        Class,            // User-defined or built-in classes
        Interface,        // Interface types
        GenericParameter, // Type parameter (T, E, K, V)
        Array,            // Array types (wraps element type)
        Void,             // void return type
        Null,             // null type
        Lambda            // Lambda/function type
    };

    /**
     * Represents a type constraint for bounded generics.
     * Example: T extends Comparable, T implements Serializable
     */
    struct TypeConstraint
    {
        enum class Kind : uint8_t
        {
            Extends,    // Class inheritance constraint
            Implements  // Interface implementation constraint
        };

        Kind kind;
        UnifiedTypePtr boundType;  // The type that must be extended/implemented

        TypeConstraint(Kind k, UnifiedTypePtr bound)
            : kind(k), boundType(std::move(bound)) {}

        bool operator==(const TypeConstraint& other) const;
        bool operator!=(const TypeConstraint& other) const { return !(*this == other); }

        std::string toString() const;
    };

    /**
     * Unified type representation used across parser, compiler, and runtime.
     * Immutable after construction - all modifications return new instances.
     *
     * Replaces:
     * - ast::GenericType (AST layer)
     * - String-based type parsing (now handled by TypeSubstitutionService)
     * - Scattered ValueType usage (runtime layer)
     */
    class UnifiedType : public std::enable_shared_from_this<UnifiedType>
    {
    private:
        TypeKind kind;
        std::string name;  // Type name: "Int", "Container", "T", etc.
        std::vector<UnifiedTypePtr> typeArguments;  // For Container<String, Int>
        std::vector<TypeConstraint> constraints;    // For T extends Comparable
        bool nullable;
        errors::SourceLocation location;

        // Private constructor - use factory methods
        UnifiedType(TypeKind k, std::string n,
                   std::vector<UnifiedTypePtr> args,
                   std::vector<TypeConstraint> cons,
                   bool isNullable,
                   errors::SourceLocation loc);

    public:
        // Disable copy/move to ensure uniqueness through factory methods
        UnifiedType(const UnifiedType&) = delete;
        UnifiedType& operator=(const UnifiedType&) = delete;
        UnifiedType(UnifiedType&&) = delete;
        UnifiedType& operator=(UnifiedType&&) = delete;

        ~UnifiedType() = default;

        // ============== Factory Methods ==============

        /**
         * Creates a primitive type (int, float, bool, string).
         */
        static UnifiedTypePtr primitive(value::ValueType vt);

        /**
         * Creates a class type, optionally with type arguments.
         * @param name Class name (e.g., "Container", "List")
         * @param args Type arguments for generics (e.g., [String] for List<String>)
         */
        static UnifiedTypePtr classType(const std::string& name,
                                        std::vector<UnifiedTypePtr> args = {});

        /**
         * Creates an interface type, optionally with type arguments.
         */
        static UnifiedTypePtr interfaceType(const std::string& name,
                                            std::vector<UnifiedTypePtr> args = {});

        /**
         * Creates a generic type parameter (T, E, K, V).
         * @param name Parameter name
         * @param constraints Optional bounds (T extends Comparable)
         */
        static UnifiedTypePtr genericParam(const std::string& name,
                                           std::vector<TypeConstraint> constraints = {});

        /**
         * Creates an array type wrapping the element type.
         * Array<String> represented as arrayOf(classType("String"))
         */
        static UnifiedTypePtr arrayOf(UnifiedTypePtr elementType);

        /**
         * Creates void type for methods with no return.
         */
        static UnifiedTypePtr voidType();

        /**
         * Creates null type.
         */
        static UnifiedTypePtr nullType();

        /**
         * Creates lambda/function type.
         * @param name Optional name for named lambdas
         */
        static UnifiedTypePtr lambdaType(const std::string& name = "");

        // ============== Type Queries ==============

        TypeKind getKind() const { return kind; }
        const std::string& getName() const { return name; }
        const std::vector<UnifiedTypePtr>& getTypeArguments() const { return typeArguments; }
        const std::vector<TypeConstraint>& getConstraints() const { return constraints; }
        bool isNullable() const { return nullable; }
        const errors::SourceLocation& getLocation() const { return location; }

        /**
         * Checks if this is a generic type parameter (T, E, etc.)
         */
        bool isGenericParameter() const { return kind == TypeKind::GenericParameter; }

        /**
         * Checks if this type has type arguments (Container<T>).
         */
        bool isParameterized() const { return !typeArguments.empty(); }

        /**
         * Checks if this type or any nested type contains unresolved generic parameters.
         */
        bool containsGenericParameters() const;

        /**
         * Checks if this is a primitive type.
         */
        bool isPrimitive() const { return kind == TypeKind::Primitive; }

        /**
         * Checks if this is a class type.
         */
        bool isClass() const { return kind == TypeKind::Class; }

        /**
         * Checks if this is an interface type.
         */
        bool isInterface() const { return kind == TypeKind::Interface; }

        /**
         * Checks if this is an array type.
         */
        bool isArray() const { return kind == TypeKind::Array; }

        /**
         * Checks if this is void type.
         */
        bool isVoid() const { return kind == TypeKind::Void; }

        /**
         * Checks if this is null type.
         */
        bool isNull() const { return kind == TypeKind::Null; }

        /**
         * Checks if this is a lambda type.
         */
        bool isLambda() const { return kind == TypeKind::Lambda; }

        /**
         * Gets element type for array types.
         * @throws std::runtime_error if not an array type
         */
        UnifiedTypePtr getElementType() const;

        /**
         * Converts to legacy ValueType enum for backward compatibility.
         */
        value::ValueType toValueType() const;

        // ============== Type Operations ==============

        /**
         * Creates a new type with type parameter substitutions applied.
         * Immutable - returns new instance.
         *
         * @param substitutions Map from parameter names to concrete types
         * @return New type with substitutions applied
         * @throws std::runtime_error on circular substitution detected
         */
        UnifiedTypePtr substitute(const TypeSubstitutionMap& substitutions) const;

        /**
         * Creates a nullable version of this type.
         */
        UnifiedTypePtr makeNullable() const;

        /**
         * Creates a non-nullable version of this type.
         */
        UnifiedTypePtr makeNonNullable() const;

        /**
         * Creates a copy with a new source location.
         */
        UnifiedTypePtr withLocation(const errors::SourceLocation& loc) const;

        // ============== Comparison & Identity ==============

        /**
         * Checks structural equality with another type.
         */
        bool equals(const UnifiedTypePtr& other) const;

        /**
         * Computes hash for use in hash containers.
         */
        size_t hash() const;

        /**
         * Returns canonical string representation for type identity.
         * Normalized form: "Container<String,Int>" (no spaces, consistent ordering)
         */
        std::string toCanonicalString() const;

        /**
         * Returns human-readable string representation.
         * May include spaces: "Container<String, Int>"
         */
        std::string toString() const;

        bool operator==(const UnifiedType& other) const;
        bool operator!=(const UnifiedType& other) const { return !(*this == other); }

    private:
        static constexpr int MAX_SUBSTITUTION_DEPTH = 50;

        UnifiedTypePtr substituteInternal(
            const TypeSubstitutionMap& substitutions,
            std::unordered_set<std::string>& visited,
            int depth) const;

        static std::string valueTypeToString(value::ValueType vt);
        static std::string kindToString(TypeKind k);
    };

    /**
     * Hash functor for UnifiedTypePtr for use in unordered containers.
     */
    struct UnifiedTypeHash
    {
        size_t operator()(const UnifiedTypePtr& type) const
        {
            return type ? type->hash() : 0;
        }
    };

    /**
     * Equality functor for UnifiedTypePtr for use in unordered containers.
     */
    struct UnifiedTypeEqual
    {
        bool operator()(const UnifiedTypePtr& a, const UnifiedTypePtr& b) const
        {
            if (!a && !b) return true;
            if (!a || !b) return false;
            return a->equals(b);
        }
    };
}
