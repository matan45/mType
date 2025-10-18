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

        // Convert to string for error messages
        std::string toString() const {
            if (isInterface()) {
                return interfaceName.value();
            } else if (isClass()) {
                return className.value();
            } else {
                // Return basic type name
                switch (basicType) {
                    case ValueType::INT: return "int";
                    case ValueType::FLOAT: return "float";
                    case ValueType::BOOL: return "bool";
                    case ValueType::STRING: return "string";
                    case ValueType::VOID: return "void";
                    case ValueType::LAMBDA: return "lambda";
                    case ValueType::NULL_TYPE: return "null";
                    case ValueType::OBJECT: return "object";
                    case ValueType::ARRAY: return "array";
                    default: return "unknown";
                }
            }
        }

        // Equality comparison
        bool operator==(const ParameterType& other) const {
            return basicType == other.basicType &&
                   interfaceName == other.interfaceName &&
                   className == other.className;
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