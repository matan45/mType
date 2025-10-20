#pragma once
#include "../../../ast/ASTNode.hpp"
#include "../../../ast/nodes/functions/FunctionNode.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include "../../runtime/utils/TypeConverter.hpp"
#include <memory>

namespace vm::compiler::registration
{
    /**
     * Pre-registers function signatures before compilation
     * This allows forward references and mutual recursion
     */
    class FunctionRegistrar
    {
    public:
        explicit FunctionRegistrar(bytecode::BytecodeProgram& program);
        ~FunctionRegistrar() = default;

        // Main registration method
        void registerFunctionSignatures(ast::ASTNode* node);

    private:
        bytecode::BytecodeProgram& program;

        // Helper methods
        void registerSingleFunction(ast::FunctionNode* functionNode);
    };
}
