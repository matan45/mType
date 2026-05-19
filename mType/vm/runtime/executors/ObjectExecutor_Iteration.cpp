#include "ObjectExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../../../value/ObjectInstancePool.hpp"

namespace vm::runtime
{
    void ObjectExecutor::handleGetIterator(const bytecode::BytecodeProgram::Instruction& instr)
    {
        value::Value collectionValue = context.stackManager->pop();

        utils::checkNullReceiver(instr, collectionValue, context, environment, "get iterator from", "collection");

        // Arrays get a built-in ArrayIteratorHelper instance.
        if (value::isNativeArray(collectionValue)) {
            auto classRegistry = environment->getClassRegistry();
            auto iteratorHelperClass = classRegistry->findClass("ArrayIteratorHelper");

            if (!iteratorHelperClass) {
                utils::ErrorLocationHelper::throwRuntimeError(context,
                    "ArrayIteratorHelper class not found - required for array iteration");
            }

            auto iteratorInstance = value::ObjectInstancePool::getInstance().acquire(iteratorHelperClass);

            // Local array lets us hand a span without allocating a vector.
            value::Value oneArg[] = { collectionValue };
            auto constructor = iteratorHelperClass->findConstructorByTypes(
                std::span<const value::Value>(oneArg, 1));
            if (!constructor) {
                utils::ErrorLocationHelper::throwRuntimeError(context,
                    "ArrayIteratorHelper constructor not found");
            }

            // Set the array field directly (constructor would do this, but bypass
            // it to skip the bytecode call). ArrayIteratorHelper fields: array, index.
            iteratorInstance->setField("array", collectionValue);
            iteratorInstance->setField("index", static_cast<int64_t>(0));

            context.stackManager->push(iteratorInstance);
            return;
        }

        if (!value::isAnyObject(collectionValue)) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "GET_ITERATOR requires an object instance");
        }

        // MYT-208: pass the receiver Value directly so OBJECT and STACK_OBJECT
        // collections both dispatch correctly.
        std::string methodName = "iterator";
        invokeInstanceMethod(collectionValue, methodName, std::span<const value::Value>{}, 0);
    }

    void ObjectExecutor::handleIteratorHasNext(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // MYT-156: pop the iterator. The for-each compiler emits a fresh
        // LOAD_LOCAL iter before each iterator op, so popping here keeps the
        // operand stack balanced (peeking leaked one slot per iteration and
        // blocked OSR with OPERAND_STACK_NOT_EMPTY).
        value::Value iteratorValue = context.stackManager->pop();

        utils::checkNullReceiver(instr, iteratorValue, context, environment, "call hasNext() on", "iterator");

        if (!value::isAnyObject(iteratorValue)) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "ITERATOR_HAS_NEXT requires an iterator instance");
        }

        // MYT-208: pass receiver Value directly (covers OBJECT + STACK_OBJECT).
        std::string methodName = "hasNext";
        invokeInstanceMethod(iteratorValue, methodName, std::span<const value::Value>{}, 0);
    }

    void ObjectExecutor::handleIteratorNext(const bytecode::BytecodeProgram::Instruction& instr)
    {
        // MYT-156: pop the iterator (see handleIteratorHasNext).
        value::Value iteratorValue = context.stackManager->pop();

        utils::checkNullReceiver(instr, iteratorValue, context, environment, "call next() on", "iterator");

        if (!value::isAnyObject(iteratorValue)) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "ITERATOR_NEXT requires an iterator instance");
        }

        // MYT-208: pass receiver Value directly (covers OBJECT + STACK_OBJECT).
        std::string methodName = "next";
        invokeInstanceMethod(iteratorValue, methodName, std::span<const value::Value>{}, 0);
    }

    void ObjectExecutor::handleIteratorClose(const bytecode::BytecodeProgram::Instruction& /*instr*/)
    {
        value::Value iteratorValue = context.stackManager->pop();

        if (value::isNullType(iteratorValue)) {
            return;
        }

        if (!value::isAnyObject(iteratorValue)) {
            return;
        }

        // MYT-208: pass receiver Value directly (covers OBJECT + STACK_OBJECT).
        std::string methodName = "close";
        try {
            invokeInstanceMethod(iteratorValue, methodName, std::span<const value::Value>{}, 0);
            // close() returns void; pop the return value (matches try-with-resources).
            context.stackManager->pop();
        }
        catch (...) {
            // Ignore exceptions during cleanup (matches try-with-resources).
        }
    }

    // MYT-274 Phase 2: structural-equality fused-opcode handlers.
    //
    // STRUCT_HASH_INT operand layout: [fieldCount, slot0, slot1, ...].
    // Pops `this`, reads each int field by index, computes Horner-style hash,
    // pushes int. Mirrors what the synthesized Horner expression body does
    // but in a single dispatch with no operand-stack churn between fields.
    void ObjectExecutor::handleStructHashInt(const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (instr.numOperands() == 0)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context, "STRUCT_HASH_INT requires fieldCount operand");
        }
        const size_t fieldCount = static_cast<size_t>(instr.inlineOperands[0]);
        if (instr.numOperands() < 1 + fieldCount)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context, "STRUCT_HASH_INT operand list truncated");
        }

        value::Value receiverValue = context.stackManager->pop();
        utils::checkNullReceiver(instr, receiverValue, context, environment, "compute hashCode of", "this");

        int64_t h = 1;

        // Hot path is ObjectInstance (heap); ValueObject is secondary for
        // value-class receivers (none in Phase 1's gate, but cheap to support
        // for future-proofing).
        auto* objRaw = value::asObjectInstanceRaw(receiverValue);
        if (objRaw && objRaw->hasFieldVector())
        {
            for (size_t i = 0; i < fieldCount; ++i)
            {
                const size_t slot = static_cast<size_t>(instr.operandAt(1 + i));
                const value::Value& fv = objRaw->getFieldByIndex(slot);
                const int64_t iv = value::isInt(fv) ? value::asInt(fv) : 0;
                h = h * 31 + iv;
            }
        }
        else if (value::isValueObject(receiverValue))
        {
            auto vobj = value::asValueObject(receiverValue);
            for (size_t i = 0; i < fieldCount; ++i)
            {
                const size_t slot = static_cast<size_t>(instr.operandAt(1 + i));
                const value::Value& fv = vobj->getFieldByIndex(slot);
                const int64_t iv = value::isInt(fv) ? value::asInt(fv) : 0;
                h = h * 31 + iv;
            }
        }

        context.stackManager->push(value::Value(h));
    }

    // STRUCT_EQ_INT operand layout: [ownerClassNameIdx, fieldCount, slot0, slot1, ...].
    // Pops `other` (Object), pops `this`. Push false if other is null or
    // not an instance of ownerClass. Otherwise compare each int field
    // pairwise; push false on mismatch, true when all match.
    void ObjectExecutor::handleStructEqInt(const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (instr.numOperands() < 2)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context, "STRUCT_EQ_INT requires owner-class name and fieldCount");
        }
        const std::string& ownerClassName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        const size_t fieldCount = static_cast<size_t>(instr.inlineOperands[1]);
        if (instr.numOperands() < 2 + fieldCount)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context, "STRUCT_EQ_INT operand list truncated");
        }

        value::Value otherValue = context.stackManager->pop();
        value::Value thisValue = context.stackManager->pop();

        // Null other → not equal. Class-mismatch → not equal. The synthesized
        // body's `isClassOf <Owner>` guard collapses into this check.
        if (value::isNullType(otherValue))
        {
            context.stackManager->push(value::Value(false));
            return;
        }

        auto* otherRaw = value::asObjectInstanceRaw(otherValue);
        auto* thisRaw = value::asObjectInstanceRaw(thisValue);
        if (!otherRaw || !thisRaw || !otherRaw->hasFieldVector() || !thisRaw->hasFieldVector())
        {
            context.stackManager->push(value::Value(false));
            return;
        }

        // Class compatibility: `other` must be an instance of ownerClassName
        // (the class on which the synthesized equals lives). isSubclass
        // covers the same-class case (subclass-of-self == true).
        const std::string& otherClassName = otherRaw->getClassDefinition()->getName();
        if (otherClassName != ownerClassName && !isSubclass(otherClassName, ownerClassName))
        {
            context.stackManager->push(value::Value(false));
            return;
        }

        for (size_t i = 0; i < fieldCount; ++i)
        {
            const size_t slot = static_cast<size_t>(instr.operandAt(2 + i));
            const value::Value& a = thisRaw->getFieldByIndex(slot);
            const value::Value& b = otherRaw->getFieldByIndex(slot);
            const int64_t ai = value::isInt(a) ? value::asInt(a) : 0;
            const int64_t bi = value::isInt(b) ? value::asInt(b) : 0;
            if (ai != bi)
            {
                context.stackManager->push(value::Value(false));
                return;
            }
        }

        context.stackManager->push(value::Value(true));
    }
}
