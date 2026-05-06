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
        // MYT-282: precise array form for ARRAY-tag parameters, e.g. "int[]",
        // "Animal[]", "string[][]". Set by ParameterType::forArray; read by
        // toString() and TypeConversionUtils::getTypeDisplayName(ParameterType)
        // so signatures, error messages, and overload-resolution keys
        // distinguish `int[]` from `string[]` instead of collapsing both to
        // "array".
        std::optional<std::string> arrayElementTypeName;
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

        // MYT-282: array constructor — preserves the precise array form so
        // signatures and error messages don't coarsen to "array". The full
        // form (e.g. "int[]", "Animal[][]") is stored verbatim; callers
        // that need the element type strip the trailing "[]" via
        // getElementTypeName().
        static ParameterType forArray(const std::string& fullArrayTypeName) {
            ParameterType param(ValueType::ARRAY);
            param.arrayElementTypeName = fullArrayTypeName;
            return param;
        }

        // Check if this parameter is a typed array.
        bool isArray() const {
            return basicType == ValueType::ARRAY && arrayElementTypeName.has_value();
        }

        // Get the array's full type name (e.g. "int[]", "Animal[][]"). Throws
        // if this is not an array parameter.
        const std::string& getArrayTypeName() const {
            if (!arrayElementTypeName.has_value()) {
                throw errors::TypeResolutionException("Parameter is not an array type");
            }
            return arrayElementTypeName.value();
        }

        // Get the element type with one set of brackets stripped (e.g. "int"
        // for "int[]", "int[]" for "int[][]"). Returns empty string if not
        // an array or the stored form is malformed. Centralizes the
        // trailing-"[]" splitting that callers would otherwise repeat.
        std::string getElementTypeName() const {
            if (!arrayElementTypeName.has_value()) return {};
            const std::string& full = arrayElementTypeName.value();
            if (full.size() < 2 ||
                full.compare(full.size() - 2, 2, "[]") != 0) {
                return {};
            }
            return full.substr(0, full.size() - 2);
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
            std::string base;
            if (isInterface()) {
                base = interfaceName.value();
            } else if (isClass()) {
                base = className.value();
            } else if (basicType == ValueType::ARRAY && arrayElementTypeName.has_value()) {
                // MYT-282: precise array form when available.
                base = arrayElementTypeName.value();
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
                   arrayElementTypeName == other.arrayElementTypeName &&
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