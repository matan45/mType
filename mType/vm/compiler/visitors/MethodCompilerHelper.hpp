#pragma once
#include "CompilerContext.hpp"
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
            std::string returnTypeStr;
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
        size_t compileMethodBodyWithFrame(ast::MethodNode* node, const MethodParameters& params, bool isStatic);
        void finalizeMethodCompilation(ast::MethodNode* node, const MethodParameters& params,
                                       size_t methodStart, size_t skipJump, size_t localCount, bool isStatic);
    };
}
