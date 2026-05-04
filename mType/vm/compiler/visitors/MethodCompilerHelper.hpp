#pragma once
#include "CompilerContext.hpp"
#include <cstddef>
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../value/ValueType.hpp"
#include <vector>
#include <string>

namespace vm::compiler::visitors
{
    /**
     * Helper class for compiling methods and constructors
     * Extracted from ClassCompiler to improve Single Responsibility Principle compliance
     * Handles: method compilation, constructor compilation, field initialization
     */
    class MethodCompilerHelper
    {
    public:
        /**
         * Structure to hold method parameter information
         */
        struct MethodParameters {
            std::vector<std::string> paramNames;
            std::vector<std::string> paramTypes;
            std::vector<bool> paramNullable;
            std::string returnTypeStr;
        };

        /**
         * Structure to hold compiled method body information
         */
        struct MethodBodyInfo {
            size_t localCount;
            std::vector<std::string> localVarNames;
        };

        explicit MethodCompilerHelper(CompilerContext& context);
        ~MethodCompilerHelper() = default;

        // Main compilation methods
        value::Value compileMethod(ast::MethodNode* node);
        value::Value compileConstructor(ast::ConstructorNode* node);
        void compileDefaultConstructor(ast::ClassNode* node);

        // Field initialization
        void initializeInstanceFields(ast::ClassNode* node);

    private:
        CompilerContext& ctx;

        // Method compilation helpers
        MethodParameters collectMethodParameters(ast::MethodNode* node, bool isStatic);
        MethodBodyInfo compileMethodBodyWithFrame(ast::MethodNode* node, const MethodParameters& params,
                                                  bool isStatic, const std::string& qualifiedMethodName);
        void finalizeMethodCompilation(ast::MethodNode* node, const MethodParameters& params,
                                       size_t methodStart, size_t skipJump, const MethodBodyInfo& bodyInfo, bool isStatic);

        // Type validation helper
        bool isValidTypeName(const std::string& typeName, const std::vector<std::string>& validGenericParams);

        // MYT-274 Phase 2: structural-equality fast emit. If the method is a
        // compiler-synthesized hashCode/equals on a class whose own instance
        // fields are all int and that has no in-program parent (so no
        // super-compose to honor), emit a single STRUCT_HASH_INT or
        // STRUCT_EQ_INT fused opcode plus RETURN_VALUE, skipping body
        // compilation entirely. Returns true if the fast path was emitted.
        bool tryEmitStructuralFastBody(ast::MethodNode* node);
    };
}
