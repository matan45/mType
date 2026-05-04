#pragma once
#include "InlineCacheTable.hpp"
#include <cstdint>
#include "../../../value/ValueType.hpp"
#include <utility>

namespace vm::jit::ic
{
    class TypeFeedbackCollector
    {
    public:
        explicit TypeFeedbackCollector(InlineCacheTable& table);

        // Record observed types for a binary operation at the given bytecode offset
        void recordBinaryOp(size_t instructionOffset,
                            const value::Value& left,
                            const value::Value& right);

        // Check if a site should be specialized (consistent types seen >= threshold)
        bool shouldSpecialize(size_t instructionOffset) const;

        // Get the dominant type pair for a site
        std::pair<ObservedType, ObservedType> getDominantTypes(size_t instructionOffset) const;

        // MYT-163: IC-table accessor used by JIT-emit-time speculative inlining
        // (tryEmitInlinedMethodCall reads MethodInlineCache state to decide
        // whether a call site is MONO and eligible).
        InlineCacheTable& getICTable() const { return icTable; }

        static constexpr uint32_t SPECIALIZATION_THRESHOLD = 50;

    private:
        InlineCacheTable& icTable;

        static ObservedType classifyValue(const value::Value& val);
    };
}
