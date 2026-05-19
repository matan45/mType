#include "ObjectInstanceHelper.hpp"
#include "../VirtualMachine.hpp"
#include "../../../value/SmallArgsBuffer.hpp"
#include "../../../value/ObjectInstancePool.hpp"
#include "../../../value/IntegerCache.hpp"
#include "../../../value/BoolCache.hpp"
#include "../../../value/FloatCache.hpp"
#include "../../../value/StringCache.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include "../../profiler/ProfilerHookHelper.hpp"
#include "../../../errors/SourceLocation.hpp"

namespace vm::runtime
{
    namespace
    {
        value::PrimitiveTypeTag specializableTypeNameToTag(const std::string& typeName)
        {
            if (typeName == "Int" || typeName == "int") return value::PrimitiveTypeTag::INT;
            if (typeName == "Float" || typeName == "float") return value::PrimitiveTypeTag::FLOAT;
            if (typeName == "Bool" || typeName == "bool") return value::PrimitiveTypeTag::BOOL;
            if (typeName == "String" || typeName == "string") return value::PrimitiveTypeTag::STRING;
            return value::PrimitiveTypeTag::NONE;
        }

        bool isStdHashMapClass(const runtimeTypes::klass::ClassDefinition* classDef)
        {
            if (!classDef) return false;
            return classDef->hasField("keys") &&
                   classDef->hasField("values") &&
                   classDef->hasField("hashes") &&
                   classDef->hasField("capacity") &&
                   classDef->hasField("count") &&
                   classDef->hasMethod("containsKey") &&
                   classDef->hasMethod("getKeys") &&
                   classDef->hasMethod("getValues");
        }

        bool isStdHashSetClass(const runtimeTypes::klass::ClassDefinition* classDef)
        {
            if (!classDef) return false;
            return classDef->hasField("elements") &&
                   classDef->hasField("hashes") &&
                   classDef->hasField("capacity") &&
                   classDef->hasField("count") &&
                   classDef->hasMethod("contains") &&
                   classDef->hasMethod("toArray");
        }

        std::shared_ptr<runtimeTypes::klass::ClassDefinition> resolveKeyClass(
            const std::shared_ptr<environment::registry::ClassRegistry>& classRegistry,
            const std::string& keyTypeName)
        {
            if (!classRegistry) return nullptr;
            // Strip generic suffix if present (e.g. "Foo<Int>" → "Foo"). Phase 1
            // requires the K class itself to be non-generic per
            // SpecializedCollectionStorage::isSpecializableShape; a parameterized
            // K is rejected by that gate anyway, so a base-name lookup is fine.
            std::string base = keyTypeName;
            size_t lt = base.find('<');
            if (lt != std::string::npos) base = base.substr(0, lt);
            return classRegistry->findClass(base);
        }

        bool tryAttachShapeCollection(
            runtimeTypes::klass::ObjectInstance* instance,
            value::SpecializedCollectionStorage::Kind kind,
            const std::shared_ptr<environment::registry::ClassRegistry>& classRegistry,
            const std::string& keyTypeName,
            size_t initialCapacity)
        {
            auto keyClassDef = resolveKeyClass(classRegistry, keyTypeName);
            if (!value::SpecializedCollectionStorage::isSpecializableShape(keyClassDef.get())) return false;
            auto shape = value::SpecializedCollectionStorage::buildShapeDescriptor(keyClassDef);
            if (shape.empty()) return false;
            instance->attachSpecializedShapeCollection(kind, std::move(shape), initialCapacity);
            return true;
        }

        void attachSpecializedCollectionIfNeeded(
            runtimeTypes::klass::ObjectInstance* instance,
            const std::string& baseClassName,
            const std::unordered_map<std::string, std::string>& genericTypeBindings,
            size_t initialCapacity,
            const std::shared_ptr<environment::registry::ClassRegistry>& classRegistry)
        {
            if (!instance) return;

            if (baseClassName == "HashMap")
            {
                if (!isStdHashMapClass(instance->getClassDefinitionRaw())) return;
                auto it = genericTypeBindings.find("K");
                if (it == genericTypeBindings.end()) return;
                const std::string& keyTypeName = it->second;
                auto tag = specializableTypeNameToTag(keyTypeName);
                if (value::SpecializedCollectionStorage::isSpecializableKeyTag(tag))
                {
                    instance->attachSpecializedCollection(
                        value::SpecializedCollectionStorage::Kind::MAP,
                        tag,
                        initialCapacity);
                    return;
                }
                tryAttachShapeCollection(
                    instance,
                    value::SpecializedCollectionStorage::Kind::MAP,
                    classRegistry,
                    keyTypeName,
                    initialCapacity);
                return;
            }

            if (baseClassName == "HashSet")
            {
                if (!isStdHashSetClass(instance->getClassDefinitionRaw())) return;
                auto it = genericTypeBindings.find("T");
                if (it == genericTypeBindings.end()) return;
                const std::string& keyTypeName = it->second;
                auto tag = specializableTypeNameToTag(keyTypeName);
                if (value::SpecializedCollectionStorage::isSpecializableKeyTag(tag))
                {
                    instance->attachSpecializedCollection(
                        value::SpecializedCollectionStorage::Kind::SET,
                        tag,
                        initialCapacity);
                    return;
                }
                tryAttachShapeCollection(
                    instance,
                    value::SpecializedCollectionStorage::Kind::SET,
                    classRegistry,
                    keyTypeName,
                    initialCapacity);
            }
        }
    }

    void ObjectInstanceHelper::invokeConstructor(const value::Value& receiverValue,
                                                const std::string& baseClassName,
                                                std::span<const value::Value> args)
    {
        // MYT-208: tag-aware unwrap. raw is the only handle used for data
        // access (ensureFieldVector, setFieldByIndex, etc.). The original
        // Value is preserved so we push it back as `this` (preserving the
        // tag) and so we tag-branch the new CallFrame.
        auto* instance = value::asObjectInstanceRaw(receiverValue);
        size_t argCount = args.size();
        auto classRegistry = environment->getClassRegistry();
        auto classDef = classRegistry->findClass(baseClassName);

        auto constructor = classDef->findConstructorByTypes(args);
        if (!constructor) {
            bool hasAnyConstructor = !classDef->getConstructors().empty();

            if (argCount == 0 && !hasAnyConstructor) {
                // MYT-208: push the original tagged Value (preserves STACK_OBJECT)
                // instead of constructing an OBJECT Value.
                context.stackManager->push(receiverValue);
                return;
            }

            throw errors::RuntimeException("No constructor found for " + baseClassName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        auto accessContext = createAccessContext(baseClassName, false);
        validation::AccessValidator::validateConstructorAccess(baseClassName, constructor->getAccessModifier(), accessContext);

        // Phase 3 (allocation perf): trivial ctor — body is strictly
        // `this.F_k = param_k`. Copy args directly into instance fields and
        // leave the instance on the operand stack (matching the default-ctor
        // early-return convention above). Skips CallFrame setup + ctor
        // bytecode interpret loop entirely. Debugger/profiler will not see
        // an explicit function-entry event for these ctors — that's the
        // cost of the fast path.
        //
        // Phase 4: use pre-resolved field indices + setFieldByIndex to
        // bypass string-hashed fieldValues::insert on every write.
        if (constructor->isTrivialConstructor())
        {
            const auto& indexed = constructor->getTrivialFieldIndexAssignments();
            instance->ensureFieldVector();
            for (const auto& [fieldIdx, paramIdx] : indexed)
            {
                if (paramIdx < argCount)
                {
                    instance->setFieldByIndex(fieldIdx, args[paramIdx]);
                }
            }
            // MYT-208: preserve the original tag (OBJECT or STACK_OBJECT).
            context.stackManager->push(receiverValue);
            return;
        }

        std::string typeSignature = constructor->getTypeSignature();

        std::string constructorName = typeSignature.empty()
            ? baseClassName + "::<init>"
            : baseClassName + "::<init>/" + typeSignature;
        auto funcMetadata = context.program->getFunction(constructorName);
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        if (!funcMetadata) {
            const auto& loaded = vm->getLoadedPrograms();
            for (size_t i = 0; i < loaded.size(); ++i) {
                auto libFunc = loaded[i]->getFunction(constructorName);
                if (libFunc) {
                    funcMetadata = libFunc;
                    targetProgramIndex = i;
                    targetProgram = loaded[i];
                    break;
                }
            }
        }

        if (!funcMetadata) {
            throw errors::RuntimeException("Constructor '" + constructorName + "' for class '" + baseClassName +
                                         "' has no bytecode. All constructors must be compiled to bytecode for VM execution.");
        }

        CallFrame frame;
        frame.returnAddress = context.instructionPointer;
        frame.frameBase = context.stackManager->size();
        frame.localBase = context.stackManager->size();
        // MYT-197: intern on the target program.
        frame.functionName = targetProgram->internFrameName(constructorName);
        // MYT-208: tag-branch `this` ownership for the ctor frame. STACK_OBJECT
        // routes through thisInstanceRaw (lifetime owned by the caller frame's
        // stackObjects); OBJECT keeps the shared_ptr in thisInstance.
        if (value::isStackObject(receiverValue))
        {
            frame.thisInstanceRaw = instance;
        }
        else
        {
            frame.thisInstance = value::asObject(receiverValue);
        }
        frame.definingClassName = baseClassName;
        frame.programIndex = targetProgramIndex;

        context.pushCallFrame(std::move(frame));
        context.stats.functionCalls++;

        if (targetProgram != context.program) {
            context.program = targetProgram;
        }

        vm::profiler::ProfilerHookHelper::onFunctionEntry(constructorName);

        if (debugger::DebugHookHelper::isDebuggingEnabled()) {
            auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
            if (sourceLoc) {
                errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                debugger::DebugHookHelper::enterFunctionHook(constructorName, errorsLoc);
            } else {
                auto ctorStartLoc = context.program->getSourceLocation(funcMetadata->startOffset);
                if (ctorStartLoc) {
                    errors::SourceLocation errorsLoc(ctorStartLoc->filename, ctorStartLoc->line, ctorStartLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(constructorName, errorsLoc);
                } else {
                    debugger::DebugHookHelper::enterFunctionHook(constructorName, errors::SourceLocation());
                }
            }
        }

        // MYT-208: push the receiver Value preserving its tag so the ctor
        // body's LOAD_LOCAL 0 reads STACK_OBJECT for stack-promoted ctors.
        context.stackManager->push(receiverValue);
        for (size_t i = 0; i < argCount; ++i) {
            context.stackManager->push(args[i]);
        }

        context.instructionPointer = funcMetadata->startOffset - 1;
    }

    void ObjectInstanceHelper::handleNewObject(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.numOperands() < 2) {
            throw errors::RuntimeException("NEW_OBJECT requires 2 operands: class name index and arg count");
        }

        const std::string& fullClassName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        size_t argCount = instr.inlineOperands[1];

        std::unordered_map<std::string, std::string> genericTypeBindings;
        std::string baseClassName = parseGenericTypeArguments(fullClassName, genericTypeBindings);

        // MYT-196: pop constructor arguments into a small-buffer-optimized scratch.
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i) {
            args[i - 1] = context.stackManager->pop();
        }

        // PHASE 2 OPTIMIZATION: Integer Caching — if creating Int object with single
        // int argument in cacheable range, use cached instance.
        if (baseClassName == "Int" && argCount == 1 && value::isInt(args[0])) {
            int intValue = static_cast<int>(value::asInt(args[0]));

            if (value::IntegerCache::isCacheable(intValue)) {
                auto classRegistry = environment->getClassRegistry();
                auto intClassDef = classRegistry ? classRegistry->findClass("Int") : nullptr;

                if (intClassDef) {
                    auto cachedInstance = value::IntegerCache::getInt(intValue, intClassDef);

                    if (cachedInstance) {
                        // Cache hit! Skip constructor invocation; the cached
                        // object is already properly initialized.
                        // MYT-208: invokeConstructor takes a Value receiver.
                        invokeConstructor(value::Value(cachedInstance), baseClassName, args.span());
                        return;
                    }
                }
            }
        }

        // Bool caching: only two values, so cache hit rate is 100%.
        if (baseClassName == "Bool" && argCount == 1 && value::isBool(args[0])) {
            auto classRegistry = environment->getClassRegistry();
            auto boolClassDef = classRegistry ? classRegistry->findClass("Bool") : nullptr;
            if (boolClassDef) {
                auto cachedInstance = value::BoolCache::getBool(value::asBool(args[0]), boolClassDef);
                if (cachedInstance) {
                    // MYT-208: invokeConstructor takes a Value receiver.
                    invokeConstructor(value::Value(cachedInstance), baseClassName, args.span());
                    return;
                }
            }
        }

        // MYT-272: Float caching for hand-picked common constants
        // ({0.0, 1.0, -1.0, 0.5, -0.5, 2.0}). Bitwise-compared so NaN
        // never aliases a cached entry.
        if (baseClassName == "Float" && argCount == 1 && value::isFloat(args[0])) {
            double floatValue = value::asFloat(args[0]);
            if (value::FloatCache::isCacheable(floatValue)) {
                auto classRegistry = environment->getClassRegistry();
                auto floatClassDef = classRegistry ? classRegistry->findClass("Float") : nullptr;
                if (floatClassDef) {
                    auto cachedInstance = value::FloatCache::getFloat(floatValue, floatClassDef);
                    if (cachedInstance) {
                        invokeConstructor(value::Value(cachedInstance), baseClassName, args.span());
                        return;
                    }
                }
            }
        }

        // MYT-272: String wrapper caching keyed on StringPool interned id.
        // Empty string is a singleton; non-empty content within StringPool
        // intern range is cached up to StringCache::kMaxEntries with FIFO eviction.
        if (baseClassName == "String" && argCount == 1) {
            std::string strValue;
            bool gotString = false;
            // MYT-317: SSO-aware. Accept all three string forms.
            if (value::isAnyString(args[0])) {
                strValue = std::string(value::asStringView(args[0]));
                gotString = true;
            }
            if (gotString) {
                auto classRegistry = environment->getClassRegistry();
                auto stringClassDef = classRegistry ? classRegistry->findClass("String") : nullptr;
                if (stringClassDef) {
                    auto cachedInstance = value::StringCache::getString(strValue, stringClassDef);
                    if (cachedInstance) {
                        invokeConstructor(value::Value(cachedInstance), baseClassName, args.span());
                        return;
                    }
                }
            }
        }

        size_t collectionInitialCapacity = 32;
        if ((baseClassName == "HashMap" || baseClassName == "HashSet") &&
            argCount == 1 && value::isInt(args[0]) && value::asInt(args[0]) > 0)
        {
            collectionInitialCapacity = static_cast<size_t>(value::asInt(args[0]));
        }

        auto instance = createObjectInstance(baseClassName, genericTypeBindings);
        attachSpecializedCollectionIfNeeded(
            instance.get(),
            baseClassName,
            genericTypeBindings,
            collectionInitialCapacity,
            environment->getClassRegistry());

        // Invoke constructor using the class definition's actual name
        // (handles aliases: "MyInt" resolves to the same ClassDef as "Int",
        // but constructor bytecode is registered under "Int::<init>").
        std::string actualClassName = instance->getClassDefinition()->getName();
        // MYT-208: NEW_OBJECT path always produces an OBJECT-tagged Value.
        invokeConstructor(value::Value(instance), actualClassName, args.span());
    }

    void ObjectInstanceHelper::handleNewStack(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.numOperands() < 2) {
            throw errors::RuntimeException("NEW_STACK requires 2 operands: class name index and arg count");
        }

        const std::string& fullClassName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        size_t argCount = instr.inlineOperands[1];

        std::unordered_map<std::string, std::string> genericTypeBindings;
        std::string baseClassName = parseGenericTypeArguments(fullClassName, genericTypeBindings);

        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i) {
            args[i - 1] = context.stackManager->pop();
        }

        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) {
            throw errors::RuntimeException("Class registry not available");
        }
        auto classDef = classRegistry->findClass(baseClassName);
        if (!classDef) {
            throw errors::RuntimeException("Class not found: " + baseClassName);
        }

        // MYT-208: per-frame stackObjects cap. Above this threshold the
        // owning frame is accumulating promoted allocations faster than they
        // can be released — typical of top-level for-loop workloads
        // (object_alloc.mt). The pool can never recycle a slot across loop
        // iterations in that pattern, so every acquireRaw hits the pool's
        // miss path (operator new + placement new), losing the SlotDeleter
        // recycling that NEW_OBJECT uses. Falling back to the heap path
        // restores recycling. The cap matches CallFrame::kStackObjectsCap
        // (the inline array's fixed size).
        if (context.callStack.empty() ||
            context.callStack.back().stackObjectsCount >= CallFrame::kStackObjectsCap)
        {
            auto instance = createObjectInstance(baseClassName, genericTypeBindings);
            size_t collectionInitialCapacity = 32;
            if ((baseClassName == "HashMap" || baseClassName == "HashSet") &&
                argCount == 1 && value::isInt(args[0]) && value::asInt(args[0]) > 0)
            {
                collectionInitialCapacity = static_cast<size_t>(value::asInt(args[0]));
            }
            attachSpecializedCollectionIfNeeded(
                instance.get(),
                baseClassName,
                genericTypeBindings,
                collectionInitialCapacity,
                classRegistry);
            std::string actualClassName = instance->getClassDefinition()->getName();
            invokeConstructor(value::Value(instance), actualClassName, args.span());
            return;
        }

        // MYT-208: pool-borrowed raw allocation. Skips shared_ptr control
        // block, SlotDeleter, GC register and atomic retain/release for the
        // object's lifetime. Lifetime is owned by the *current* (caller)
        // frame's stackObjects array — pushed there BEFORE invokeConstructor
        // so that an exception thrown inside the ctor body still releases
        // the slot via ExceptionHandler's frame-teardown path.
        size_t collectionInitialCapacity = 32;
        if ((baseClassName == "HashMap" || baseClassName == "HashSet") &&
            argCount == 1 && value::isInt(args[0]) && value::asInt(args[0]) > 0)
        {
            collectionInitialCapacity = static_cast<size_t>(value::asInt(args[0]));
        }

        auto* raw = value::ObjectInstancePool::getInstance().acquireRaw(classDef, genericTypeBindings);
        initializeObjectFields(raw, classDef);
        attachSpecializedCollectionIfNeeded(
            raw,
            baseClassName,
            genericTypeBindings,
            collectionInitialCapacity,
            classRegistry);

        // The cap-check above guarantees the inline array has room.
        context.callStack.back().tryPushStackObject(raw);

        std::string actualClassName = raw->getClassDefinition()->getName();
        // MYT-208: build a STACK_OBJECT-tagged Value for the receiver. The
        // tag flows through invokeConstructor's tag-branched frame setup so
        // the new ctor frame uses thisInstanceRaw, and the operand stack push
        // for `this` stays refcount-free.
        invokeConstructor(value::makeStackObjectValue(raw), actualClassName, args.span());
    }
}
