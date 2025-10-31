#pragma once
#include "../../../ast/ASTNode.hpp"
#include "../../../ast/nodes/functions/FunctionNode.hpp"
#include "../../../environment/Environment.hpp"
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
        explicit FunctionRegistrar(
            std::shared_ptr<environment::Environment> environment,
            bytecode::BytecodeProgram& program);
        ~FunctionRegistrar() = default;

        // Main registration method
        void registerFunctionSignatures(ast::ASTNode* node);

        // Validate @Throw annotations after classes are registered
        void validateThrowAnnotations(ast::ASTNode* node);

    private:
        std::shared_ptr<environment::Environment> environment;
        bytecode::BytecodeProgram& program;

        // Helper methods
        void registerSingleFunction(ast::FunctionNode* functionNode);
        void validateSingleFunctionThrow(ast::FunctionNode* functionNode);
    };
}
