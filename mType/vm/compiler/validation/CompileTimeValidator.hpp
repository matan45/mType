#pragma once
#include "../../../ast/ASTNode.hpp"
#include "../../../environment/Environment.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include <string>
#include <memory>
#include <vector>

namespace vm::compiler::validation
{
    /**
     * Compile-time validation helper
     * Validates that functions, methods, variables, and classes exist at compile time
     * Prevents runtime "not found" errors by catching them during compilation
     */
    class CompileTimeValidator
    {
    public:
        explicit CompileTimeValidator(
            std::shared_ptr<environment::Environment> env,
            bytecode::BytecodeProgram& prog
        );
        ~CompileTimeValidator() = default;

        // Function validation
        void validateFunctionExists(const std::string& functionName, const ast::SourceLocation& location);
        void validateStaticMethodExists(const std::string& className, const std::string& methodName,
                                       size_t argCount, const ast::SourceLocation& location);

        // Method validation
        void validateInstanceMethodExists(const std::string& className, const std::string& methodName,
                                         size_t argCount, const ast::SourceLocation& location);
        void validateConstructorExists(const std::string& className, size_t argCount,
                                      const ast::SourceLocation& location);

        // Variable validation
        void validateVariableExists(const std::string& varName, const ast::SourceLocation& location);

        // Class validation
        void validateClassExists(const std::string& className, const ast::SourceLocation& location);
        void validateParentClassExists(const std::string& parentClassName, const ast::SourceLocation& location);

        // Generic type validation
        void validateTypeIsNotRawGeneric(const std::string& typeName, const ast::SourceLocation& location);

        // Method implementation validation (called after class registration)
        void validateAllMethodsHaveBytecode(const std::string& className, const ast::SourceLocation& location);

    private:
        std::shared_ptr<environment::Environment> environment;
        bytecode::BytecodeProgram& program;

        // Helper to get fully qualified method name
        std::string getQualifiedMethodName(const std::string& className, const std::string& methodName,
                                          size_t argCount, bool isStatic = false);
    };
}
