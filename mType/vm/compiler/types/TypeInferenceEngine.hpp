#pragma once
#include "../../../ast/ASTNode.hpp"
#include "../../../value/ValueType.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include "../../../environment/Environment.hpp"
#include "../variables/VariableTracker.hpp"
#include "../variables/GlobalVariableRegistry.hpp"
#include <string>
#include <memory>

namespace vm::compiler::types
{
    /**
     * Performs type inference for expressions
     * Determines the ValueType and class name for any expression node
     */
    class TypeInferenceEngine
    {
    public:
        TypeInferenceEngine(
            const bytecode::BytecodeProgram& program,
            std::shared_ptr<environment::Environment> environment,
            const variables::VariableTracker& variableTracker,
            const variables::GlobalVariableRegistry& globalRegistry
        );

        ~TypeInferenceEngine() = default;

        // Type inference methods
        value::ValueType inferExpressionType(ast::ASTNode* node) const;
        std::string inferExpressionClassName(ast::ASTNode* node) const;

    private:
        const bytecode::BytecodeProgram& program;
        std::shared_ptr<environment::Environment> environment;
        const variables::VariableTracker& variableTracker;
        const variables::GlobalVariableRegistry& globalRegistry;
    };
}
