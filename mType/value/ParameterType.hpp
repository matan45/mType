#pragma once
#include "ValueType.hpp"
#include <string>
#include <optional>
#include "../errors/TypeResolutionException.hpp"

namespace value {
    /**
     * Enhanced parameter type that stores both the basic ValueType and interface information
     */
    struct ParameterType {
        ValueType basicType;                    // INT, FLOAT, BOOL, STRING, OBJECT, etc.
        std::optional<std::string> interfaceName;  // Interface name if basicType is OBJECT
        std::optional<std::string> className;      // Class name if basicType is OBJECT
        bool nullable = false;                     // Whether this parameter accepts null

        // Default constructor for basic types
        explicit ParameterType(ValueType type)
            : basicType(type) {}

        // Constructor for interface types
        ParameterType(ValueType type, const std::string& interface)
            : basicType(type), interfaceName(interface) {}

        // Constructor for class types
        static ParameterType forClass(const std::string& className) {
            ParameterType param(ValueType::OBJECT);
            param.className = className;
            return param;
        }

        // Constructor for interface types
        static ParameterType forInterface(const std::string& interfaceName) {
            ParameterType param(ValueType::OBJECT);
            param.interfaceName = interfaceName;
            return param;
        }

        // Check if this parameter represents an interface
        bool isInterface() const {
            return basicType == ValueType::OBJECT && interfaceName.has_value();
        }

        // Check if this parameter represents a specific class
        bool isClass() const {
            return basicType == ValueType::OBJECT && className.has_value() && !interfaceName.has_value();
        }

        // Get the interface name (throws if not an interface)
        const std::string& getInterfaceName() const {
            if (!interfaceName.has_value()) {
                throw errors::TypeResolutionException("Parameter is not an interface type");
            }
            return interfaceName.value();
        }

        // Get the class name (throws if not a class)
        const std::string& getClassName() const {
            if (!className.has_value()) {
                throw errors::TypeResolutionException("Parameter is not a class type");
            }
            return className.value();
        }

        // Fluent setter for nullable
        ParameterType& withNullable(bool n) { nullable = n; return *this; }

        // Convert to string for error messages
        std::string toString() const {
            std::string base;
            if (isInterface()) {
                base = interfaceName.value();
            } else if (isClass()) {
                base = className.value();
            } else {
                switch (basicType) {
                    case ValueType::INT: base = "int"; break;
                    case ValueType::FLOAT: base = "float"; break;
                    case ValueType::BOOL: base = "bool"; break;
                    case ValueType::STRING: base = "string"; break;
                    case ValueType::VOID: base = "void"; break;
                    case ValueType::LAMBDA: base = "lambda"; break;
                    case ValueType::NULL_TYPE: base = "null"; break;
                    case ValueType::OBJECT: base = "object"; break;
                    case ValueType::ARRAY: base = "array"; break;
                    default: base = "unknown"; break;
                }
            }
            if (nullable) base += "?";
            return base;
        }

        // Equality comparison
        bool operator==(const ParameterType& other) const {
            return basicType == other.basicType &&
                   interfaceName == other.interfaceName &&
                   className == other.className &&
                   nullable == other.nullable;
        }

        // Implicit conversion to ValueType for backward compatibility
        operator ValueType() const {
            return basicType;
        }

        // NOTE: Static conversion helpers have been moved to ParameterTypeConverter
        // for better separation of concerns (Single Responsibility Principle).
        // Include "ParameterTypeConverter.hpp" to use:
        // - ParameterTypeConverter::fromValueTypeVector()
        // - ParameterTypeConverter::toValueTypeVector()
        // - ParameterTypeConverter::fromValueType()
        // - ParameterTypeConverter::toValueType()
    };
}

// Forward compatibility: Include converter for users who expect these methods here
#include "ParameterTypeConverter.hpp"