#include "ObjectExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../context/SharedStackFramePool.hpp"
#include "../../../value/ObjectInstancePool.hpp"
#include "../../../value/SmallArgsBuffer.hpp"
#include "../../../value/NativeArray.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include "../../profiler/ProfilerHookHelper.hpp"
#include "../../../errors/SourceLocation.hpp"
#include "../../../environment/registry/SignatureUtils.hpp"

namespace vm::runtime
{
    void ObjectExecutor::invokeLambdaMethod(std::shared_ptr<BytecodeLambda> lambda,
                                           std::span<const value::Value> args,
                                           const std::string& methodName) {
        size_t lambdaStart = lambda->instructionPointer;
        size_t paramCount = lambda->parameterCount;

        if (args.size() != paramCount) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Lambda expects " + std::to_string(paramCount) +
                " arguments but got " + std::to_string(args.size()));
        }

        CallFrame frame;
        frame.returnAddress = context.instructionPointer;
        frame.frameBase = context.stackManager->size();
        frame.localBase = context.stackManager->size();
        // Use the lambda's unique function name (e.g., __lambda_0) for metadata/exception table lookup.
        // MYT-197: 4-byte handle copy; fall back to an interned display name
        // only for lambdas created outside handleLambda.
        if (lambda->functionName != bytecode::INVALID_FN_HANDLE) {
            frame.functionName = lambda->functionName;
        } else {
            std::string fallback = lambda->creatingClassName.empty()
                ? "<lambda>"
                : lambda->creatingClassName + "::<lambda>";
            frame.functionName = context.program->internFrameName(fallback);
        }
        frame.thisInstance = lambda->capturedThis;
        frame.originatingLambda = lambda;
        frame.definingClassName = lambda->creatingClassName;

        context.pushCallFrame(std::move(frame));

        const std::string& frameNameStr = context.program->getFrameName(frame.functionName);
        vm::profiler::ProfilerHookHelper::onFunctionEntry(frameNameStr);

        if (debugger::DebugHookHelper::isDebuggingEnabled()) {
            auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
            if (sourceLoc) {
                errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                debugger::DebugHookHelper::enterFunctionHook(frameNameStr, errorsLoc);
            } else {
                auto lambdaStartLoc = context.program->getSourceLocation(lambdaStart);
                if (lambdaStartLoc) {
                    errors::SourceLocation errorsLoc(lambdaStartLoc->filename, lambdaStartLoc->line, lambdaStartLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(frameNameStr, errorsLoc);
                } else {
                    debugger::DebugHookHelper::enterFunctionHook(frameNameStr, errors::SourceLocation());
                }
            }
        }

        // Create a SharedStackFrame for this lambda invocation to support nested closures.
        // Link it to the parent frame so nested lambdas can access parent variables.
        auto newSharedFrame = makePooledFrame();
        newSharedFrame->parentFrame = lambda->capturedFrame;
        if (!context.callStack.empty()) {
            context.callStack.back().sharedFrame = newSharedFrame;
        }

        // MYT-197: O(1) handle lookup for parameter type information.
        auto* lambdaMetadata = context.program->getFunctionMeta(lambda->functionName);

        // Push arguments as locals (indices 0, 1, 2, ...).
        // Also register them by name in SharedStackFrame for nested lambda capture.
        for (size_t i = 0; i < args.size(); ++i) {
            value::Value argValue = args[i];

            // Auto-box primitive arguments if lambda expects boxed types
            if (lambdaMetadata && i < lambdaMetadata->parameterTypes.size()) {
                std::string expectedType = lambdaMetadata->parameterTypes[i];

                bool needsBoxing = false;
                std::string boxClassName;

                if (expectedType == "Int" && value::isInt(argValue)) {
                    needsBoxing = true;
                    boxClassName = "Int";
                }
                else if (expectedType == "Float" && (value::isFloat(argValue) || value::isInt(argValue))) {
                    needsBoxing = true;
                    boxClassName = "Float";
                }
                else if (expectedType == "Bool" && value::isBool(argValue)) {
                    needsBoxing = true;
                    boxClassName = "Bool";
                }
                else if (expectedType == "String" && value::isAnyString(argValue)) {
                    // MYT-317: STRING_INLINE also auto-boxes into String.
                    needsBoxing = true;
                    boxClassName = "String";
                }

                if (needsBoxing) {
                    auto classDef = environment->findClass(boxClassName);
                    if (classDef) {
                        if (classDef->isValueClass()) {
                            auto valueObj = std::make_shared<value::ValueObject>(classDef);
                            valueObj->setField("value", argValue);
                            argValue = value::Value(valueObj);
                        } else {
                            std::unordered_map<std::string, std::string> emptyBindings;
                            auto boxedInstance = value::ObjectInstancePool::getInstance().acquire(classDef, emptyBindings);
                            boxedInstance->setField("value", argValue);
                            argValue = boxedInstance;
                        }
                    }
                }
            }

            context.stackManager->push(argValue);

            if (i < lambda->parameterNames.size()) {
                std::string paramName = lambda->parameterNames[i];
                if (!paramName.empty()) {
                    newSharedFrame->setLocal(paramName, i, argValue);
                }
            }
        }

        // Push captured variables onto stack (as locals after parameters).
        // Read current values from shared frame (reference capture semantics).
        // IMPORTANT: Do NOT register them in the new SharedStackFrame — they must be
        // accessed through the parent chain to always read the latest values.
        size_t capturedCount = 0;
        if (lambda->capturedFrame) {
            for (size_t i = 0; i < lambda->capturedSlots.size(); ++i) {
                size_t slot = lambda->capturedSlots[i];

                // Slot-based lookup avoids name collisions, allowing multiple
                // variables with the same name to coexist.
                value::Value capturedValue = lambda->capturedFrame->getLocal(slot);

                context.stackManager->push(capturedValue);
                capturedCount++;
            }
        }

        // Reserve additional local variable slots (for return-value temporaries etc).
        if (lambdaMetadata) {
            size_t pushedSlots = args.size() + capturedCount;
            if (lambdaMetadata->localCount > pushedSlots) {
                size_t additionalLocals = lambdaMetadata->localCount - pushedSlots;
                for (size_t i = 0; i < additionalLocals; ++i) {
                    context.stackManager->push(std::monostate{});
                }
            }
        }

        context.instructionPointer = lambdaStart - 1;
    }

    void ObjectExecutor::invokeValueObjectMethod(
        std::shared_ptr<value::ValueObject> valueObj,
        const std::string& methodName,
        std::span<const value::Value> args,
        size_t argCount)
    {
        auto classDef = valueObj->getClassDefinition();
        if (!classDef) {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "Value object has no class definition");
        }

        // Materialise the receiver as a temp ObjectInstance so the method body
        // gets the in-place mutation semantics that value-class tests rely on
        // (see inline_value_object_write_skip.mt — `this.n = this.n + 1` must
        // be visible within the call). The previous hot loop ran setField per
        // declared field; loadFromValueObject batch-copies the vector and
        // syncs fieldValues in one pass — no GC write barrier, no per-field
        // hashmap probes, no gcRegistered branch.
        auto tempInstance = value::ObjectInstancePool::getInstance().acquire(classDef);
        tempInstance->loadFromValueObject(*valueObj);

        // MYT-208: invokeInstanceMethod takes a Value receiver. Wrap the temp
        // shared_ptr in an OBJECT-tagged Value (always heap, never stack-
        // promoted — value-class temp instance lifetime is tied to shared_ptr).
        invokeInstanceMethod(value::Value(tempInstance), methodName, args, argCount);
    }

    void ObjectExecutor::invokeArrayObjectMethod(
        const value::Value& arr,
        const std::string& methodName,
        std::span<const value::Value> args)
    {
        // The compiler emits CALL_METHOD with a class-qualified name like
        // "Object::toString" when the receiver's static type is Object.
        // Strip the qualifier so the matching below is on the simple name —
        // mirrors trySpecializedCollectionCall at this same site.
        const std::string simpleName =
            runtimeTypes::klass::SignatureUtils::extractSimpleName(methodName);

        // Element-type display name + dimensional shape, used for
        // toString / getClass. NativeArray exposes elementTypeName directly;
        // FlatMultiArray / SparseMultiArray / FlatMultiObjectArray fall back
        // to "array" until they grow an accessor.
        std::string elementName = "array";
        size_t arrSize = 0;
        if (value::isNativeArray(arr)) {
            auto na = value::asNativeArray(arr);
            elementName = na->getElementTypeName();
            arrSize = na->size();
        } else if (value::isFlatMultiArray(arr)) {
            auto fa = value::asFlatMultiArray(arr);
            arrSize = fa->size();
        } else if (value::isSparseMultiArray(arr)) {
            auto sa = value::asSparseMultiArray(arr);
            arrSize = sa->size();
        }

        if (simpleName == "toString") {
            std::string repr = elementName + "[" + std::to_string(arrSize) + "]";
            context.stackManager->push(value::Value(std::move(repr)));
            return;
        }
        if (simpleName == "getClass") {
            std::string cls = elementName + "[]";
            context.stackManager->push(value::Value(std::move(cls)));
            return;
        }
        if (simpleName == "hashCode") {
            // Identity hash based on bridge pointer — matches Java array
            // semantics (default Object.hashCode is identity-based). Mask the
            // high bit so the conversion to int64_t is well-defined even when
            // the address sits above INT64_MAX.
            constexpr uintptr_t kSignMask = static_cast<uintptr_t>(0x7FFFFFFFFFFFFFFFULL);
            uintptr_t addr = reinterpret_cast<uintptr_t>(arr.rawBridge());
            int64_t h = static_cast<int64_t>(addr & kSignMask);
            context.stackManager->push(value::Value(h));
            return;
        }
        if (simpleName == "equals") {
            // Reference equality — two arrays are equal iff they share the
            // same backing bridge. Element-wise comparison is a separate
            // helper (Arrays.equals in Java terms) and not part of Object.
            bool eq = false;
            if (args.size() >= 1) {
                const value::Value& other = args[0];
                eq = (other.tag() == value::ValueType::ARRAY) &&
                     (other.rawBridge() == arr.rawBridge());
            }
            context.stackManager->push(value::Value(eq));
            return;
        }

        utils::ErrorLocationHelper::throwUserException(context, environment,
            "RuntimeException",
            "method '" + simpleName + "' not found on array");
    }

    bool ObjectExecutor::tryDispatchSpecializedCollectionCall(
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (instr.numOperands() < 2 || !context.stackManager) return false;

        const std::string& rawMethodName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        const std::string methodName =
            runtimeTypes::klass::SignatureUtils::extractSimpleName(rawMethodName);
        const size_t argCount = static_cast<size_t>(instr.inlineOperands[1]);

        if (context.stackManager->size() <= argCount) return false;

        value::Value objectValue = context.stackManager->peek(argCount);
        if (!value::isAnyObject(objectValue)) return false;

        auto* instance = value::asObjectInstanceRaw(objectValue);
        if (!instance) return false;

        auto* storage = instance->getSpecializedCollection();
        if (!storage || !storage->isSpecializedMethod(methodName, argCount)) return false;

        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i)
        {
            args[i - 1] = context.stackManager->pop();
        }
        context.stackManager->pop();

        using Kind = value::SpecializedCollectionStorage::Kind;
        if (storage->getKind() == Kind::MAP)
        {
            if (methodName == "put")
            {
                value::Value oldValue;
                if (!storage->put(args[0], args[1], oldValue))
                {
                    oldValue = nullptr;
                }
                context.stackManager->push(oldValue);
                return true;
            }
            if (methodName == "get")
            {
                value::Value result;
                if (!storage->get(args[0], result)) result = nullptr;
                context.stackManager->push(result);
                return true;
            }
            if (methodName == "containsKey")
            {
                context.stackManager->push(storage->containsKey(args[0]));
                return true;
            }
            if (methodName == "remove")
            {
                context.stackManager->push(storage->remove(args[0]));
                return true;
            }
            if (methodName == "getKeys" || methodName == "toArray")
            {
                context.stackManager->push(storage->materializeKeys(environment.get()));
                return true;
            }
            if (methodName == "getValues")
            {
                context.stackManager->push(storage->materializeValues());
                return true;
            }
            if (methodName == "containsValue")
            {
                context.stackManager->push(storage->containsStoredValue(args[0]));
                return true;
            }
        }
        else
        {
            if (methodName == "add")
            {
                context.stackManager->push(storage->add(args[0]));
                return true;
            }
            if (methodName == "contains")
            {
                context.stackManager->push(storage->contains(args[0]));
                return true;
            }
            if (methodName == "remove")
            {
                context.stackManager->push(storage->remove(args[0]));
                return true;
            }
            if (methodName == "toArray")
            {
                context.stackManager->push(storage->materializeKeys(environment.get()));
                return true;
            }
        }

        if (methodName == "size")
        {
            context.stackManager->push(static_cast<int64_t>(storage->size()));
            return true;
        }
        if (methodName == "empty")
        {
            context.stackManager->push(storage->empty());
            return true;
        }
        if (methodName == "clear")
        {
            storage->clear();
            context.stackManager->push(std::monostate{});
            return true;
        }

        context.stackManager->push(nullptr);
        return true;
    }
}
