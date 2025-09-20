#pragma once

#include "../value/ValueType.hpp"
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <unordered_map>

namespace ast
{
    /**
     * Represents a generic type that can be either a concrete type or a generic type parameter.
     * Supports nested generics like Array<Map<K,V>> and type parameter references like T, E.
     */
    class GenericType
    {
    private:
        // Either a concrete type or a generic type parameter name
        std::variant<value::ValueType, std::string> baseType;
        // Type arguments for parameterized types (e.g., Array<T> has [T])
        std::vector<std::shared_ptr<GenericType>> typeArguments;

    public:
        /**
         * Constructor for concrete types (int, string, bool, etc.)
         * @param type The concrete ValueType
         */
        explicit GenericType(value::ValueType type) : baseType(type) {}

        /**
         * Constructor for generic type parameters (T, E, K, V, etc.)
         * @param genericName The name of the generic type parameter
         */
        explicit GenericType(const std::string& genericName) : baseType(genericName) {}

        /**
         * Constructor for parameterized concrete types (Array<T>, Map<K,V>)
         * @param type The base ValueType (e.g., ARRAY, MAP)
         * @param args Vector of type arguments
         */
        GenericType(value::ValueType type, const std::vector<std::shared_ptr<GenericType>>& args)
            : baseType(type), typeArguments(args) {}

        /**
         * Constructor for parameterized generic types (Box<T> where Box is generic)
         * @param genericName The name of the generic type
         * @param args Vector of type arguments
         */
        GenericType(const std::string& genericName, const std::vector<std::shared_ptr<GenericType>>& args)
            : baseType(genericName), typeArguments(args) {}

        /**
         * Copy constructor
         */
        GenericType(const GenericType& other)
            : baseType(other.baseType) {
            for (const auto& arg : other.typeArguments) {
                typeArguments.push_back(std::make_shared<GenericType>(*arg));
            }
        }

        /**
         * Assignment operator
         */
        GenericType& operator=(const GenericType& other) {
            if (this != &other) {
                baseType = other.baseType;
                typeArguments.clear();
                for (const auto& arg : other.typeArguments) {
                    typeArguments.push_back(std::make_shared<GenericType>(*arg));
                }
            }
            return *this;
        }

        /**
         * Checks if this represents a generic type parameter (T, E, etc.)
         * @return true if this is a type parameter, false if concrete type
         */
        bool isGenericParameter() const {
            return std::holds_alternative<std::string>(baseType);
        }

        /**
         * Checks if this type has type arguments (Array<T>, Map<K,V>)
         * @return true if parameterized, false otherwise
         */
        bool isParameterized() const {
            return !typeArguments.empty();
        }

        /**
         * Gets the concrete ValueType (only valid for concrete types)
         * @return The ValueType enum value
         * @throws std::runtime_error if called on generic parameter
         */
        value::ValueType getConcreteType() const;

        /**
         * Gets the generic parameter name (only valid for generic parameters)
         * @return The parameter name (T, E, etc.)
         * @throws std::runtime_error if called on concrete type
         */
        std::string getGenericName() const;

        /**
         * Gets the base type name (either ValueType name or generic parameter name)
         * @return String representation of the base type
         */
        std::string getBaseTypeName() const;

        /**
         * Gets the type arguments for parameterized types
         * @return Vector of type arguments
         */
        const std::vector<std::shared_ptr<GenericType>>& getTypeArguments() const {
            return typeArguments;
        }

        /**
         * Sets the type arguments for parameterized types
         * @param args Vector of type arguments
         */
        void setTypeArguments(const std::vector<std::shared_ptr<GenericType>>& args) {
            typeArguments = args;
        }

        /**
         * Adds a single type argument
         * @param arg The type argument to add
         */
        void addTypeArgument(std::shared_ptr<GenericType> arg) {
            typeArguments.push_back(arg);
        }

        /**
         * Gets the number of type arguments
         * @return Count of type arguments
         */
        size_t getTypeArgumentCount() const {
            return typeArguments.size();
        }

        /**
         * Converts this generic type to a string representation
         * @return String like "T", "int", "Array<T>", "Map<string, int>"
         */
        std::string toString() const;

        /**
         * Checks if two generic types are equal
         * @param other The other type to compare with
         * @return true if types are equivalent
         */
        bool equals(const GenericType& other) const;

        /**
         * Equality operator
         */
        bool operator==(const GenericType& other) const {
            return equals(other);
        }

        /**
         * Inequality operator
         */
        bool operator!=(const GenericType& other) const {
            return !equals(other);
        }

        /**
         * Creates a copy of this GenericType with type parameter substitutions
         * @param substitutions Map from type parameter names to concrete types
         * @return New GenericType with substitutions applied
         */
        std::shared_ptr<GenericType> substitute(
            const std::unordered_map<std::string, std::shared_ptr<GenericType>>& substitutions) const;

        /**
         * Static factory methods for common types
         */
        static std::shared_ptr<GenericType> createInt() {
            return std::make_shared<GenericType>(value::ValueType::INT);
        }

        static std::shared_ptr<GenericType> createString() {
            return std::make_shared<GenericType>(value::ValueType::STRING);
        }

        static std::shared_ptr<GenericType> createBool() {
            return std::make_shared<GenericType>(value::ValueType::BOOL);
        }

        static std::shared_ptr<GenericType> createFloat() {
            return std::make_shared<GenericType>(value::ValueType::FLOAT);
        }

        static std::shared_ptr<GenericType> createVoid() {
            return std::make_shared<GenericType>(value::ValueType::VOID);
        }

        static std::shared_ptr<GenericType> createObject() {
            return std::make_shared<GenericType>(value::ValueType::OBJECT);
        }

        static std::shared_ptr<GenericType> createGenericParameter(const std::string& name) {
            return std::make_shared<GenericType>(name);
        }

        static std::shared_ptr<GenericType> createArray(std::shared_ptr<GenericType> elementType) {
            return std::make_shared<GenericType>(value::ValueType::ARRAY, std::vector<std::shared_ptr<GenericType>>{elementType});
        }

        static std::shared_ptr<GenericType> createMap(std::shared_ptr<GenericType> keyType, std::shared_ptr<GenericType> valueType) {
            return std::make_shared<GenericType>(value::ValueType::MAP, std::vector<std::shared_ptr<GenericType>>{keyType, valueType});
        }
    };
}