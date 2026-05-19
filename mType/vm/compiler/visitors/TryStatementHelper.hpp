#pragma once
#include "CompilerContext.hpp"
#include <cstddef>
#include <cstdint>
#include "../../../ast/nodes/statements/TryNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../value/ValueType.hpp"

namespace vm::compiler::visitors
{
    /**
     * Handles compilation of try-catch-finally statements
     * Extracted from ControlFlowCompiler to maintain Single Responsibility Principle
     * Manages exception handling bytecode generation including:
     * - Try block setup and compilation
     * - Catch handler registration and compilation
     * - Finally block compilation with nested try-finally support
     */
    class TryStatementHelper
    {
    public:
        explicit TryStatementHelper(CompilerContext& context);
        ~TryStatementHelper() = default;

        value::Value compileTry(ast::TryNode* node);

    private:
        CompilerContext& ctx;

        struct CatchBlockRange {
            size_t startIP;
            size_t endIP;
        };

        std::vector<CatchBlockRange> compileCatchBlocks(const std::vector<std::unique_ptr<ast::CatchNode>>& catchBlocks,
                               size_t tryBeginOffset, size_t tryEndOffset, uint32_t nestingLevel);

        void compileFinallyBlock(ast::BlockNode* finallyBlock, size_t tryBeginOffset,
                                size_t tryEndOffset, uint32_t nestingLevel, bool noCatchBlocks,
                                const std::vector<CatchBlockRange>& catchRanges);
    };
}
