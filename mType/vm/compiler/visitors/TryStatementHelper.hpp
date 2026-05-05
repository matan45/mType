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

        /**
         * Compile a try-catch-finally statement
         * @param node The try statement node to compile
         * @return Empty monostate (compilation produces bytecode, not runtime value)
         */
        value::Value compileTry(ast::TryNode* node);

    private:
        CompilerContext& ctx;

        /**
         * Compile all catch blocks for a try statement
         * @param catchBlocks The catch blocks to compile
         * @param tryBeginOffset Offset where TRY_BEGIN was emitted
         * @param tryEndOffset Offset where try block ends (first TRY_END)
         * @param nestingLevel Nesting level for exception table
         */
        struct CatchBlockRange {
            size_t startIP;
            size_t endIP;
        };

        std::vector<CatchBlockRange> compileCatchBlocks(const std::vector<std::unique_ptr<ast::CatchNode>>& catchBlocks,
                               size_t tryBeginOffset, size_t tryEndOffset, uint32_t nestingLevel);

        /**
         * Compile a finally block if present
         * @param finallyBlock The finally block to compile (can be nullptr)
         * @param tryBeginOffset Offset where TRY_BEGIN was emitted
         * @param tryEndOffset Offset where try block ends (first TRY_END)
         * @param nestingLevel Nesting level for exception table
         * @param noCatchBlocks True if there are no catch blocks (finally-only)
         */
        void compileFinallyBlock(ast::BlockNode* finallyBlock, size_t tryBeginOffset,
                                size_t tryEndOffset, uint32_t nestingLevel, bool noCatchBlocks,
                                const std::vector<CatchBlockRange>& catchRanges);
    };
}
