#pragma once
#include "../../../environment/Environment.hpp"
#include <cstddef>
#include "../../../ast/ASTNode.hpp"
#include "../../../value/ParameterType.hpp"
#include "../../../errors/SourceLocation.hpp"
#include "../visitors/CompilerContext.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace vm::compiler::overload
{
    /**
     * Helper class for resolving method and function overloads during bytecode compilation
     * Uses OverloadResolver to select the best matching overload based on argument types
     * Generates mangled names for bytecode emission
     */
    class OverloadResolutionHelper
    {
    public:
        explicit OverloadResolutionHelper(visitors::CompilerContext& context);
        ~OverloadResolutionHelper() = default;

        /**
         * Resolves instance method overload and returns the mangled name
         * @param className The class containing the method
         * @param methodName The method name (without signature)
         * @param arguments The call arguments for type inference
         * @param location Source location for error reporting
         * @param hasGenericTypeArgs Whether the call has explicit generic type arguments
         * @param genericTypeArgCount Number of generic type arguments provided (if any)
         * @return Mangled method name (e.g., "add/int,int") or plain name if single overload
         */
        std::string resolveInstanceMethodOverload(
            const std::string& className,
            const std::string& methodName,
            const std::vector<std::unique_ptr<ast::ASTNode>>& arguments,
            const ast::SourceLocation& location,
            bool hasGenericTypeArgs = false,
            size_t genericTypeArgCount = 0);

        /**
         * Resolves static method overload and returns the mangled name
         * @param className The class containing the static method
         * @param methodName The method name (without signature)
         * @param arguments The call arguments for type inference
         * @param location Source location for error reporting
         * @param hasGenericTypeArgs Whether the call has explicit generic type arguments
         * @param genericTypeArgs The actual generic type arguments provided (e.g., ["Int", "String"])
         * @return Mangled method name (e.g., "add/int,int") or plain name if single overload
         */
        std::string resolveStaticMethodOverload(
            const std::string& className,
            const std::string& methodName,
            const std::vector<std::unique_ptr<ast::ASTNode>>& arguments,
            const ast::SourceLocation& location,
            bool hasGenericTypeArgs = false,
            const std::vector<std::string>& genericTypeArgs = {});

        /**
         * Resolves global function overload and returns the mangled name
         * @param functionName The function name (without signature)
         * @param arguments The call arguments for type inference
         * @param location Source location for error reporting
         * @param hasGenericTypeArgs Whether the call has explicit generic type arguments
         * @param genericTypeArgs The actual generic type arguments provided (e.g., ["Int", "String"])
         * @return Mangled function name (e.g., "process/String,int") or plain name if single overload
         */
        std::string resolveFunctionOverload(
            const std::string& functionName,
            const std::vector<std::unique_ptr<ast::ASTNode>>& arguments,
            const ast::SourceLocation& location,
            bool hasGenericTypeArgs = false,
            const std::vector<std::string>& genericTypeArgs = {});

    private:
        visitors::CompilerContext& ctx;

        /**
         * Infers parameter types from argument expressions
         * @param arguments The AST nodes to infer types from
         * @return Vector of inferred parameter types
         */
        std::vector<value::ParameterType> inferArgumentTypes(
            const std::vector<std::unique_ptr<ast::ASTNode>>& arguments);

        /**
         * Builds a mangled name from method signature
         * @param className The class name
         * @param methodName The method name
         * @param parameters The parameter types
         * @return Mangled name (e.g., "ClassName::methodName/Type1,Type2")
         */
        std::string buildMangledMethodName(
            const std::string& className,
            const std::string& methodName,
            const std::vector<std::pair<std::string, value::ParameterType>>& parameters);

        /**
         * Builds a mangled name from function signature
         * @param functionName The function name
         * @param parameters The parameter types
         * @return Mangled name (e.g., "functionName/Type1,Type2")
         */
        std::string buildMangledFunctionName(
            const std::string& functionName,
            const std::vector<std::pair<std::string, value::ParameterType>>& parameters);

        /**
         * Substitutes generic type parameters in a type string.
         * Example: substituteTypeParameters("Wrapper<T>", {{"T", "Int"}}) => "Wrapper<Int>"
         * Promoted to a static method so both _Static and _Function TUs can call it.
         */
        static std::string substituteTypeParameters(
            const std::string& typeStr,
            const std::unordered_map<std::string, std::string>& substitutions);
    };
}
