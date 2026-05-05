#include "TypeFeedbackCollector.hpp"
#include <cstddef>
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../value/ValueShim.hpp"

namespace vm::jit::ic
{
    TypeFeedbackCollector::TypeFeedbackCollector(InlineCacheTable& table)
        : icTable(table)
    {}

    ObservedType TypeFeedbackCollector::classifyValue(const value::Value& val)
    {
        if (value::isInt(val)) return ObservedType::INT;
        if (value::isFloat(val)) return ObservedType::FLOAT;
        if (value::isBool(val)) return ObservedType::INT; // bool treated as int for specialization
        if (value::isString(val)) return ObservedType::STRING;
        if (value::isInternedString(val)) return ObservedType::STRING;
        if (value::isObject(val)) return ObservedType::OBJECT;
        return ObservedType::MIXED;
    }

    void TypeFeedbackCollector::recordBinaryOp(size_t instructionOffset,
                                                const value::Value& left,
                                                const value::Value& right)
    {
        TypeFeedback& feedback = icTable.getTypeFeedback(instructionOffset);

        if (feedback.specialized) return; // Already specialized, stop profiling

        ObservedType lType = classifyValue(left);
        ObservedType rType = classifyValue(right);

        if (feedback.executionCount == 0)
        {
            // First execution — record initial types
            feedback.leftType = lType;
            feedback.rightType = rType;
        }
        else
        {
            // Check if types are consistent
            if (feedback.leftType != lType || feedback.rightType != rType)
            {
                // Types changed — mark as mixed
                feedback.leftType = ObservedType::MIXED;
                feedback.rightType = ObservedType::MIXED;
            }
        }

        ++feedback.executionCount;
    }

    bool TypeFeedbackCollector::shouldSpecialize(size_t instructionOffset) const
    {
        if (!icTable.hasTypeFeedback(instructionOffset)) return false;

        const TypeFeedback& feedback = const_cast<InlineCacheTable&>(icTable).getTypeFeedback(instructionOffset);
        if (feedback.specialized) return false;
        if (feedback.executionCount < SPECIALIZATION_THRESHOLD) return false;

        // Only specialize if we've seen consistent non-mixed types
        return feedback.leftType != ObservedType::NONE &&
               feedback.leftType != ObservedType::MIXED &&
               feedback.rightType != ObservedType::NONE &&
               feedback.rightType != ObservedType::MIXED;
    }

    std::pair<ObservedType, ObservedType> TypeFeedbackCollector::getDominantTypes(size_t instructionOffset) const
    {
        if (!icTable.hasTypeFeedback(instructionOffset))
        {
            return {ObservedType::NONE, ObservedType::NONE};
        }

        const TypeFeedback& feedback = const_cast<InlineCacheTable&>(icTable).getTypeFeedback(instructionOffset);
        return {feedback.leftType, feedback.rightType};
    }
}
