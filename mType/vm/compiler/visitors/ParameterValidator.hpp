#pragma once
#include "CompilerContext.hpp"
#include "../../../ast/ASTNode.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include "../../../runtimeTypes/klass/ConstructorDefinition.hpp"
#include <string>
#include <vector>
#include <memory>

namespace vm::compiler::visitors
{
    /**
     * Helper class for validating method and constructor parameters
     * Extracted from ClassCompiler to improve Single Responsibility Principle compliance
     * Handles: parameter count validation, type validation, generic parameter handling
     */
    class ParameterValidator
    {
    public:
        explicit ParameterValidator(CompilerContext& context);
        ~ParameterValidator() = default;

        // Main validation methods
        void validateMethodParameters(
            const std::string& methodName,
            const std::string& qualifiedName,
            const std::vector<std::unique_ptr<ast::ASTNode>>& arguments,
            const ast::SourceLocation& location);

        void validateConstructorParameters(
            const std::vector<std::unique_ptr<ast::ASTNode>>& arguments,
            const runtimeTypes::klass::ConstructorDefinition* constructor,
            const ast::SourceLocation& location,
            const std::unordered_map<std::string, std::string>& genericTypeBindings = {});

    private:
        CompilerContext& ctx;

        // Helper methods
        std::string normalizeTypeString(const std::string& typeStr);

        const bytecode::BytecodeProgram::FunctionMetadata* resolveMethodMetadata(
            const std::string& qualifiedName);

        void validateParameterCount(
            const std::string& methodName,
            const bytecode::BytecodeProgram::FunctionMetadata* methodMetadata,
            size_t argumentCount,
            const ast::SourceLocation& location);

        void validateSingleParameterType(
            const std::string& methodName,
            size_t paramIndex,
            const std::string& expectedType,
            ast::ASTNode* argument,
            const ast::SourceLocation& location,
            bool isNullable = false);

        bool validateGenericParameter(
            const std::string& methodName,
            const std::string& expectedType,
            value::ValueType argType,
            const std::string& argTypeStr,
            size_t paramIndex,
            const ast::SourceLocation& location);
    };
}
