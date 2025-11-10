#pragma once
#include "../../../value/ValueType.hpp"
#include <string>
#include <vector>

namespace vm::compiler::types
{
    /**
     * Represents expected type information that can be propagated
     * during expression compilation for bidirectional type checking.
     *
     * This enables type inference from assignment context, allowing
     * inference of generic type parameters that appear only in return types.
     *
     * Example:
     *   Box<Int> x = createBox();  // Expected type: Box<Int>
     *                              // Propagates context to infer T=Int
     */
    struct ExpectedTypeContext
    {
        // The expected ValueType (INT, FLOAT, OBJECT, etc.)
        value::ValueType expectedType;

        // For OBJECT types, the expected class name (including generics)
        // Examples: "Box<Int>", "List<String>", "Pair<Int,String>"
        std::string expectedClassName;

        // Whether this context is active/valid
        bool isActive;

        // Default constructor - creates inactive context
        ExpectedTypeContext()
            : expectedType(value::ValueType::VOID)
            , expectedClassName("")
            , isActive(false)
        {
        }

        // Constructor with type information
        ExpectedTypeContext(value::ValueType type, const std::string& className = "")
            : expectedType(type)
            , expectedClassName(className)
            , isActive(true)
        {
        }

        // Factory method for creating inactive context
        static ExpectedTypeContext none()
        {
            return ExpectedTypeContext();
        }

        // Factory method for creating context from type name
        static ExpectedTypeContext fromTypeName(value::ValueType type, const std::string& className)
        {
            return ExpectedTypeContext(type, className);
        }

        /**
         * Parse generic type arguments from expected class name
         * Example: "Box<Int>" -> ["Int"]
         *          "Pair<Int,String>" -> ["Int", "String"]
         *          "List<Box<Int>>" -> ["Box<Int>"]
         */
        std::vector<std::string> extractGenericArguments() const;

        /**
         * Get base type without generics
         * Example: "Box<Int>" -> "Box"
         *          "Pair<Int,String>" -> "Pair"
         *          "String" -> "String"
         */
        std::string getBaseClassName() const;

        /**
         * Check if the expected class name contains generic type arguments
         * Example: "Box<Int>" -> true
         *          "String" -> false
         */
        bool hasGenericArguments() const;
    };
}
