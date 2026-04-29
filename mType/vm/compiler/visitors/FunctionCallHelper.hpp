#pragma once
#include "CompilerContext.hpp"
#include "../overload/OverloadResolutionHelper.hpp"
#include "../../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../value/ValueType.hpp"
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <unordered_map>

namespace vm::compiler::visitors
{
    /**
     * Helper class for compiling function calls
     * Extracted from FunctionCompiler to improve Single Responsibility Principle compliance
     * Handles: function call compilation, parameter validation, generic type binding, method calls
     */
    class FunctionCallHelper
    {
    public:
        explicit FunctionCallHelper(CompilerContext& context);
        ~FunctionCallHelper() = default;

        // Main compilation method
        value::Value compileFunctionCall(ast::FunctionCallNode* node);

    private:
        CompilerContext& ctx;
        std::unique_ptr<overload::OverloadResolutionHelper> overloadResolver;

        // MYT-228: when set, emitBindTypeArgsIfNeeded will emit a BIND_TYPE_ARGS
        // opcode just before the terminal CALL_* emit. compileFunctionCall
        // save/restores this around the emit dispatch so nested generic calls
        // (e.g. arguments that are themselves generic calls) don't clobber
        // the outer call's bindings. Stores a copy of the map (not a pointer)
        // because nested compileFunctionCall calls may reallocate
        // ctx.genericTypeBindingStack while our emit is still pending.
        std::optional<std::unordered_map<std::string, std::string>> pendingBindings_;

        // Helper methods
        bool setupGenericTypeBindings(ast::FunctionCallNode* node, const std::string& functionName);

        std::string inferTypeFromArgument(ast::ASTNode* argument);

        // PHASE 3: Advanced type inference helpers
        void inferFromArguments(
            const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata,
            const std::vector<std::unique_ptr<ast::ASTNode>>& arguments,
            std::unordered_map<std::string, std::string>& typeBindings
        );

        void inferFromReturnType(
            const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata,
            std::unordered_map<std::string, std::string>& typeBindings,
            const ast::SourceLocation& location
        );

        void validateFunctionParameters(ast::FunctionCallNode* node, const std::string& functionName,
                                       const std::vector<std::unique_ptr<ast::ASTNode>>& arguments);

        void emitStaticMethodCall(ast::FunctionCallNode* node, const std::string& functionName,
                                 const std::vector<std::unique_ptr<ast::ASTNode>>& arguments);

        void emitMethodCallInClassContext(ast::FunctionCallNode* node, const std::string& functionName,
                                         const std::vector<std::unique_ptr<ast::ASTNode>>& arguments);

        void emitRegularFunctionCall(ast::FunctionCallNode* node, const std::string& functionName,
                                    const std::vector<std::unique_ptr<ast::ASTNode>>& arguments);

        // Phase 4: Auto-boxing helper
        void compileArgumentWithAutoBoxing(ast::ASTNode* argument,
                                          const std::string& expectedTypeName,
                                          value::ValueType expectedType);

        // MYT-228: emit BIND_TYPE_ARGS for the current call if pendingBindings_
        // is set and non-empty. Call this immediately before each terminal
        // CALL_* emit, AFTER arguments have been pushed onto the operand
        // stack (so that nested generic calls in arguments can't clobber the
        // pendingTypeArgs scratch slot before our CALL consumes it).
        // Forward-from-caller detection delegates to isTypeParamInScope
        // (GenericScopeHelper.hpp) — shared with ClassCompiler and
        // ExpressionCompiler.
        void emitBindTypeArgsIfNeeded(ast::ASTNode* node);
    };
}
