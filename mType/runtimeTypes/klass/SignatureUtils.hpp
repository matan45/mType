#pragma once

#include <string>
#include <vector>
#include "../../value/ParameterType.hpp"
#include "../../value/ValueType.hpp"

namespace runtimeTypes::klass
{
    /**
     * Utility class for generating and parsing method signatures
     * Used for method overload resolution
     *
     * Signature format: methodName/ParamType1,ParamType2,...
     * Examples:
     *   - "add/int,int"
     *   - "toString/"
     *   - "process/String,List,bool"
     *
     * Fully qualified names: ClassName::methodName/ParamType1,ParamType2
     * Examples:
     *   - "Calculator::add/int,int"
     *   - "Rectangle::<init>/int,int"
     */
    class SignatureUtils
    {
    public:
        /**
         * Generate type signature from parameter types
         * Returns empty string for no parameters
         *
         * @param parameters Vector of parameter types
         * @return Signature string (e.g., "int,String,bool")
         */
        static std::string generateTypeSignature(
            const std::vector<std::pair<std::string, value::ParameterType>>& parameters);

        /**
         * Generate type signature from ValueType list
         *
         * @param paramTypes Vector of ValueType enums
         * @return Signature string (e.g., "int,float,bool")
         */
        static std::string generateTypeSignature(
            const std::vector<value::ValueType>& paramTypes);

        /**
         * Generate type signature from type name strings
         *
         * @param typeNames Vector of type name strings
         * @return Signature string (e.g., "Int,String,Bool")
         */
        static std::string generateTypeSignatureFromNames(
            const std::vector<std::string>& typeNames);

        /**
         * Build fully qualified method name with signature
         * Format: className::methodName/typeSignature
         *
         * @param className Name of the class
         * @param methodName Name of the method
         * @param parameters Parameter types
         * @return Fully qualified name (e.g., "Calculator::add/int,int")
         */
        static std::string buildQualifiedName(
            const std::string& className,
            const std::string& methodName,
            const std::vector<std::pair<std::string, value::ParameterType>>& parameters);

        /**
         * Build fully qualified method name with type signature string
         *
         * @param className Name of the class
         * @param methodName Name of the method
         * @param typeSignature Pre-computed type signature
         * @return Fully qualified name (e.g., "Calculator::add/int,int")
         */
        static std::string buildQualifiedName(
            const std::string& className,
            const std::string& methodName,
            const std::string& typeSignature);

        /**
         * Build function name with signature (for global functions)
         * Format: functionName/typeSignature
         *
         * @param functionName Name of the function
         * @param parameters Parameter types
         * @return Mangled function name (e.g., "print/String")
         */
        static std::string buildFunctionName(
            const std::string& functionName,
            const std::vector<std::pair<std::string, value::ParameterType>>& parameters);

        /**
         * Build function name with type signature string
         *
         * @param functionName Name of the function
         * @param typeSignature Pre-computed type signature
         * @return Mangled function name (e.g., "print/String")
         */
        static std::string buildFunctionName(
            const std::string& functionName,
            const std::string& typeSignature);

        /**
         * Parse a qualified method name into components
         *
         * @param qualifiedName Fully qualified name (e.g., "Calculator::add/int,int")
         * @param className Output parameter for class name
         * @param methodName Output parameter for method name
         * @param typeSignature Output parameter for type signature
         * @return true if parsing succeeded, false otherwise
         */
        static bool parseQualifiedName(
            const std::string& qualifiedName,
            std::string& className,
            std::string& methodName,
            std::string& typeSignature);

        /**
         * Parse a function name into components
         *
         * @param mangledName Mangled function name (e.g., "print/String")
         * @param functionName Output parameter for function name
         * @param typeSignature Output parameter for type signature
         * @return true if parsing succeeded, false otherwise
         */
        static bool parseFunctionName(
            const std::string& mangledName,
            std::string& functionName,
            std::string& typeSignature);

        /**
         * Parse type signature into individual type names
         *
         * @param typeSignature Signature string (e.g., "int,String,bool")
         * @return Vector of type names
         */
        static std::vector<std::string> parseTypeSignature(
            const std::string& typeSignature);

        /**
         * Extract simple method name from qualified or mangled name
         * Examples:
         *   - "Calculator::add/int,int" -> "add"
         *   - "print/String" -> "print"
         *   - "toString" -> "toString"
         *
         * @param name Any form of method/function name
         * @return Simple method name
         */
        static std::string extractSimpleName(const std::string& name);

        /**
         * Check if a name contains a signature component
         *
         * @param name Method or function name to check
         * @return true if name includes "/..." signature part
         */
        static bool hasSignature(const std::string& name);

        /**
         * Get type name string from ParameterType
         * Handles primitives, classes, and interfaces
         *
         * @param paramType Parameter type to convert
         * @return Type name string (e.g., "int", "String", "Comparable")
         */
        static std::string getTypeName(const value::ParameterType& paramType);

    private:
        /**
         * Join vector of strings with delimiter
         */
        static std::string join(const std::vector<std::string>& elements,
                              const std::string& delimiter);

        /**
         * Split string by delimiter
         */
        static std::vector<std::string> split(const std::string& str,
                                             const std::string& delimiter);
    };

} // namespace runtimeTypes::klass
