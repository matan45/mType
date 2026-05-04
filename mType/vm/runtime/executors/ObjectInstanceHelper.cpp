#include "ObjectInstanceHelper.hpp"
#include "../../MethodSignature.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../types/TypeRegistry.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include "../../profiler/ProfilerHookHelper.hpp"
#include "../../../value/IntegerCache.hpp"
#include "../../../value/BoolCache.hpp"
#include "../../../value/FloatCache.hpp"
#include "../../../value/StringCache.hpp"
#include "../../../value/InternedString.hpp"
#include "../../../value/ObjectInstancePool.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../value/SmallArgsBuffer.hpp"
#include "../../../gc/GC.hpp"
#include <algorithm>
#include  <iostream>
namespace vm::runtime
{
    // Helper function to parse type arguments with proper bracket depth tracking
    static std::vector<std::string> parseTypeArguments(const std::string& typeArgsStr)
    {
        std::vector<std::string> typeArgs;
        std::string current;
        int depth = 0;

        for (char c : typeArgsStr)
        {
            if (c == '<')
            {
                depth++;
                current += c;
            }
            else if (c == '>')
            {
                depth--;
                current += c;
            }
            else if (c == ',' && depth == 0)
            {
                // Split at comma only if we're at depth 0 (not inside nested generics)
                if (!current.empty())
                {
                    // Trim whitespace
                    size_t start = current.find_first_not_of(" \t");
                    size_t end = current.find_last_not_of(" \t");
                    if (start != std::string::npos && end != std::string::npos)
                    {
                        typeArgs.push_back(current.substr(start, end - start + 1));
                    }
                }
                current.clear();
            }
            else
            {
                current += c;
            }
        }

        // Add the last type argument
        if (!current.empty())
        {
            // Trim whitespace
            size_t start = current.find_first_not_of(" \t");
            size_t end = current.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos)
            {
                typeArgs.push_back(current.substr(start, end - start + 1));
            }
        }

        return typeArgs;
    }

    static value::PrimitiveTypeTag specializableTypeNameToTag(const std::string& typeName)
    {
        if (typeName == "Int" || typeName == "int") return value::PrimitiveTypeTag::INT;
        if (typeName == "Float" || typeName == "float") return value::PrimitiveTypeTag::FLOAT;
        if (typeName == "Bool" || typeName == "bool") return value::PrimitiveTypeTag::BOOL;
        if (typeName == "String" || typeName == "string") return value::PrimitiveTypeTag::STRING;
        return value::PrimitiveTypeTag::NONE;
    }

    static bool isStdHashMapClass(const runtimeTypes::klass::ClassDefinition* classDef)
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

    static bool isStdHashSetClass(const runtimeTypes::klass::ClassDefinition* classDef)
    {
        if (!classDef) return false;
        return classDef->hasField("elements") &&
               classDef->hasField("hashes") &&
               classDef->hasField("capacity") &&
               classDef->hasField("count") &&
               classDef->hasMethod("contains") &&
               classDef->hasMethod("toArray");
    }

    static std::shared_ptr<runtimeTypes::klass::ClassDefinition> resolveKeyClass(
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

    static bool tryAttachShapeCollection(
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

    static void attachSpecializedCollectionIfNeeded(
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

    ObjectInstanceHelper::ObjectInstanceHelper(ExecutionContext& ctx)
        : context(ctx)
    {
    }

    std::string ObjectInstanceHelper::parseGenericTypeArguments(const std::string& fullClassName,
                                                               std::unordered_map<std::string, std::string>& genericTypeBindings)
    {
        std::string baseClassName = fullClassName;
        size_t genericStart = fullClassName.find('<');

        if (genericStart == std::string::npos) {
            return baseClassName;
        }

        baseClassName = fullClassName.substr(0, genericStart);
        size_t genericEnd = fullClassName.rfind('>');
        if (genericEnd == std::string::npos || genericEnd <= genericStart) {
            return baseClassName;
        }

        std::string typeArgsStr = fullClassName.substr(genericStart + 1, genericEnd - genericStart - 1);

        // Parse type arguments with proper bracket depth tracking
        // Correctly handles nested generics like "Container<HashMap<String, List<Int>>>"
        std::vector<std::string> typeArgs = parseTypeArguments(typeArgsStr);

        // Validate type arguments
        auto& typeRegistry = types::getGlobalTypeRegistry();
        for (const auto& typeArg : typeArgs) {
            if (typeArg.empty()) {
                throw errors::TypeException("Invalid empty type argument for generic class '" + baseClassName + "'");
            }

            if (typeArg == "void" && baseClassName == "Promise") {
                continue;
            }

            // PURE OOP: Primitives are now allowed as generic type arguments!
            // They are treated as their Box class equivalents (Int, Float, Bool, String)
            // No need to reject primitive types - they're now objects
        }

        // Map generic parameters to concrete types
        // Get the class definition to access generic parameters
        if (!context.environment) {
            throw errors::RuntimeException("Environment not available when parsing generic type arguments for class: " + baseClassName);
        }

        auto classRegistry = context.environment->getClassRegistry();
        if (!classRegistry) {
            throw errors::RuntimeException("Class registry not available when parsing generic type arguments for class: " + baseClassName);
        }

        auto classDef = classRegistry->findClass(baseClassName);
        if (!classDef) {
            throw errors::RuntimeException("Class definition not found when parsing generic type arguments for class: " + baseClassName);
        }

        const auto& genericParams = classDef->getGenericParameters();

        // Map each generic parameter to its corresponding type argument
        for (size_t i = 0; i < genericParams.size() && i < typeArgs.size(); ++i) {
            genericTypeBindings[genericParams[i].name] = typeArgs[i];
        }

        return baseClassName;
    }

    std::shared_ptr<runtimeTypes::klass::ObjectInstance> ObjectInstanceHelper::createObjectInstance(
        const std::string& baseClassName,
        const std::unordered_map<std::string, std::string>& genericTypeBindings)
    {
        auto classRegistry = context.environment->getClassRegistry();
        if (!classRegistry) {
            throw errors::RuntimeException("Class registry not available");
        }

        auto classDef = classRegistry->findClass(baseClassName);
        if (!classDef) {
            throw errors::RuntimeException("Class not found: " + baseClassName);
        }

        auto instance = value::ObjectInstancePool::getInstance().acquire(classDef, genericTypeBindings);
        // MYT-208: helper takes raw pointer — pass .get() (lifetime owned by
        // the shared_ptr returned by acquire).
        initializeObjectFields(instance.get(), classDef);

        // GC: Register the newly created object with the garbage collector
        instance->registerWithGC();

        return instance;
    }

    void ObjectInstanceHelper::initializeObjectFields(
        runtimeTypes::klass::ObjectInstance* instance,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef)
    {
        std::vector<std::shared_ptr<runtimeTypes::klass::ClassDefinition>> hierarchy;
        auto current = classDef;
        int depth = 0;
        while (current) {
            if (depth++ > 100) {
                throw errors::RuntimeException("Circular inheritance detected in class hierarchy");
            }
            hierarchy.push_back(current);  // O(1) instead of O(n)
            current = current->getParentClass();
        }
        // Reverse once at end: O(n) total instead of O(n²)
        std::reverse(hierarchy.begin(), hierarchy.end());

        // Phase 2 (allocation perf): only the top-of-hierarchy class (the one
        // being allocated) is safe to skip-default-init for. Parent fields
        // stay fully default-initialised because a child constructor may
        // read `this.<parent_field>` before calling super(), and we don't
        // know the ctor ordering for inherited state at this point.
        for (size_t i = 0; i < hierarchy.size(); ++i) {
            const auto& classInHierarchy = hierarchy[i];
            const bool isSelf = (i == hierarchy.size() - 1);
            for (const auto& [fieldName, fieldDef] : classInHierarchy->getInstanceFields()) {
                // Final fields are always written by the constructor: inline
                // initializers fire in the prologue, ctor-initialized fields
                // fire in the body. Pre-populating them here would make the
                // instance-final check in ObjectExecutor::SET_FIELD see the
                // field as already set and reject the legitimate first write.
                // FieldInitializationValidator catches uninitialized finals
                // at compile time.
                if (fieldDef->isFinal()) continue;

                // Phase 2: the compiler proved every constructor of this
                // class assigns this field before any read. Skipping avoids
                // the unordered_map<string,Value>::insert + write barrier on
                // the allocation hot path.
                if (isSelf && classInHierarchy->shouldSkipDefaultInit(fieldName)) continue;

                value::Value initialValue = fieldDef->getValue();
                if (value::isVoid(initialValue)) {
                    switch (fieldDef->getType()) {
                        case value::ValueType::INT:
                            initialValue = static_cast<int64_t>(0);
                            break;
                        case value::ValueType::FLOAT:
                            initialValue = 0.0;
                            break;
                        case value::ValueType::BOOL:
                            initialValue = false;
                            break;
                        case value::ValueType::STRING:
                            initialValue = std::string("");
                            break;
                        default:
                            // Objects, arrays, lambdas initialize to null
                            initialValue = nullptr;
                            break;
                    }
                }
                // MYT-212: route default-init through THIS class's own slot
                // (not the most-derived slot) so a child shadowing the field
                // doesn't overwrite the parent's value. setField()'s name path
                // would resolve via getFieldInHierarchy → child's slot.
                size_t ownSlot = classInHierarchy->getOwnFieldIndex(fieldName);
                if (ownSlot != SIZE_MAX) {
                    instance->setFieldByIndex(ownSlot, initialValue);
                } else {
                    // Fallback for fields not in the indexed map (defensive).
                    instance->setField(fieldName, initialValue);
                }
            }
        }
    }

    void ObjectInstanceHelper::handleSuperConstructor(const bytecode::BytecodeProgram::Instruction& instr) {
        // Operand[0] is classNameIndex, Operand[1] is argCount
        if (instr.operands.size() < 2) {
            throw errors::RuntimeException("SUPER_CONSTRUCTOR requires 2 operands (classNameIndex, argCount)");
        }

        const std::string& currentClassName = context.program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        // MYT-196: small-buffer-optimized args buffer.
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i) {
            args[i - 1] = context.stackManager->pop();
        }

        // MYT-208: accept stack-promoted `this` (NEW_STACK ctor frames).
        if (context.callStack.empty() || !context.callStack.back().getThisInstanceRaw()) {
            throw errors::RuntimeException("SUPER_CONSTRUCTOR can only be called from within an instance context");
        }

        auto& currentFrame = context.callStack.back();
        // Build a Value preserving the receiver's storage tag so the super
        // ctor frame inherits stack-promoted lifetime when applicable.
        value::Value thisValue = currentFrame.thisInstanceRaw
            ? value::makeStackObjectValue(currentFrame.thisInstanceRaw)
            : value::Value(currentFrame.thisInstance);
        auto* instanceRaw = currentFrame.getThisInstanceRaw();

        // IMPORTANT: Use the current class name from the operand, NOT the instance's class!
        // The instance might be a subclass (e.g., Dog), but we're in the Mammal constructor
        // and need to call Mammal's parent (Animal), not Dog's parent (Mammal).
        auto classDef = context.environment->getClassRegistry()->findClass(currentClassName);
        if (!classDef) {
            throw errors::RuntimeException("Current class not found: " + currentClassName);
        }

        if (!classDef->hasParentClass()) {
            throw errors::RuntimeException("Class " + classDef->getName() + " has no parent class");
        }

        std::string parentClassName = classDef->getParentClassName();

        // Extract base parent class name (strip generic type arguments)
        // e.g., "BaseContainer<T>" -> "BaseContainer"
        std::string baseParentClassName = parentClassName;
        size_t genericStart = parentClassName.find('<');
        if (genericStart != std::string::npos) {
            baseParentClassName = parentClassName.substr(0, genericStart);
        }

        auto parentClass = context.environment->getClassRegistry()->findClass(baseParentClassName);
        if (!parentClass) {
            throw errors::RuntimeException("Parent class not found: " + baseParentClassName);
        }

        // Use type-aware constructor lookup for overload resolution
        auto parentConstructor = parentClass->findConstructorByTypes(args.span());
        if (!parentConstructor) {
            throw errors::RuntimeException("No constructor found in parent class " + baseParentClassName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        // Build constructor name with type signature for overload resolution
        std::string typeSignature = parentConstructor->getTypeSignature();
        std::string constructorName = typeSignature.empty()
            ? baseParentClassName + "::<init>"
            : baseParentClassName + "::<init>/" + typeSignature;
        auto funcMetadata = context.program->getFunction(constructorName);
        // Initialize from current call frame's program index (not hardcoded 0)
        // so that if we're already executing in a library, the index is correct
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        // If not found in current program, search loaded library programs
        if (!funcMetadata && context.loadedPrograms) {
            for (size_t i = 0; i < context.loadedPrograms->size(); ++i) {
                auto libFunc = (*context.loadedPrograms)[i]->getFunction(constructorName);
                if (libFunc) {
                    funcMetadata = libFunc;
                    targetProgramIndex = i;
                    targetProgram = (*context.loadedPrograms)[i];
                    break;
                }
            }
        }

        if (funcMetadata) {
            size_t frameBase = context.stackManager->size();

            // MYT-208: push receiver Value preserving the storage tag.
            context.stackManager->push(thisValue);

            for (size_t i = 0; i < argCount; ++i) {
                context.stackManager->push(args[i]);
            }

            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            // MYT-197: intern on the target program. Constructor qualified
            // names are already registered (see registerFunction), so this is
            // a hashmap hit after first use.
            frame.functionName = targetProgram->internFrameName(constructorName);
            // MYT-208: tag-branch `this` ownership for the super ctor frame.
            if (value::isStackObject(thisValue))
            {
                frame.thisInstanceRaw = instanceRaw;
            }
            else
            {
                frame.thisInstance = currentFrame.thisInstance;
            }
            frame.definingClassName = baseParentClassName;  // Set parent class as defining class for constructor
            frame.programIndex = targetProgramIndex;

            context.pushCallFrame(std::move(frame));
            context.stats.functionCalls++;

            // Switch to the target program if it's from a library
            if (targetProgram != context.program) {
                context.program = targetProgram;
            }

            // Notify debugger of parent constructor entry
            if (debugger::DebugHookHelper::isDebuggingEnabled()) {
                auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
                if (sourceLoc) {
                    errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(constructorName, errorsLoc);
                } else {
                    // Fallback: use constructor start location
                    auto ctorStartLoc = context.program->getSourceLocation(funcMetadata->startOffset);
                    if (ctorStartLoc) {
                        errors::SourceLocation errorsLoc(ctorStartLoc->filename, ctorStartLoc->line, ctorStartLoc->column);
                        debugger::DebugHookHelper::enterFunctionHook(constructorName, errorsLoc);
                    } else {
                        debugger::DebugHookHelper::enterFunctionHook(constructorName, errors::SourceLocation());
                    }
                }
            }

            context.instructionPointer = funcMetadata->startOffset - 1;
        } else {
            throw errors::RuntimeException("Parent constructor '" + constructorName + "' has no bytecode.");
        }
    }

    void ObjectInstanceHelper::handleThisConstructor(const bytecode::BytecodeProgram::Instruction& instr) {
        // Operand[0] is classNameIndex, Operand[1] is argCount
        if (instr.operands.size() < 2) {
            throw errors::RuntimeException("THIS_CONSTRUCTOR requires 2 operands (classNameIndex, argCount)");
        }

        const std::string& currentClassName = context.program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        // MYT-196: small-buffer-optimized args buffer.
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i) {
            args[i - 1] = context.stackManager->pop();
        }

        // MYT-208: accept stack-promoted `this` (NEW_STACK ctor frames).
        if (context.callStack.empty() || !context.callStack.back().getThisInstanceRaw()) {
            throw errors::RuntimeException("THIS_CONSTRUCTOR can only be called from within an instance context");
        }

        auto& currentFrame = context.callStack.back();
        value::Value thisValue = currentFrame.thisInstanceRaw
            ? value::makeStackObjectValue(currentFrame.thisInstanceRaw)
            : value::Value(currentFrame.thisInstance);
        auto* instanceRaw = currentFrame.getThisInstanceRaw();

        // Find the current class
        auto classDef = context.environment->getClassRegistry()->findClass(currentClassName);
        if (!classDef) {
            throw errors::RuntimeException("Current class not found: " + currentClassName);
        }

        // Use type-aware constructor lookup for overload resolution
        auto targetConstructor = classDef->findConstructorByTypes(args.span());
        if (!targetConstructor) {
            throw errors::RuntimeException("No constructor found in class " + currentClassName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        // Build constructor name with type signature for overload resolution
        std::string typeSignature = targetConstructor->getTypeSignature();
        std::string constructorName = typeSignature.empty()
            ? currentClassName + "::<init>"
            : currentClassName + "::<init>/" + typeSignature;
        auto funcMetadata = context.program->getFunction(constructorName);
        // Initialize from current call frame's program index (not hardcoded 0)
        // so that if we're already executing in a library, the index is correct
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        // If not found in current program, search loaded library programs
        if (!funcMetadata && context.loadedPrograms) {
            for (size_t i = 0; i < context.loadedPrograms->size(); ++i) {
                auto libFunc = (*context.loadedPrograms)[i]->getFunction(constructorName);
                if (libFunc) {
                    funcMetadata = libFunc;
                    targetProgramIndex = i;
                    targetProgram = (*context.loadedPrograms)[i];
                    break;
                }
            }
        }

        if (funcMetadata) {
            size_t frameBase = context.stackManager->size();

            // MYT-208: push receiver Value preserving the storage tag.
            context.stackManager->push(thisValue);

            for (size_t i = 0; i < argCount; ++i) {
                context.stackManager->push(args[i]);
            }

            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            // MYT-197: intern on the target program.
            frame.functionName = targetProgram->internFrameName(constructorName);
            // MYT-208: tag-branch `this` ownership for the delegated ctor frame.
            if (value::isStackObject(thisValue))
            {
                frame.thisInstanceRaw = instanceRaw;
            }
            else
            {
                frame.thisInstance = currentFrame.thisInstance;
            }
            frame.definingClassName = currentClassName;  // Same class as defining class
            frame.programIndex = targetProgramIndex;

            context.pushCallFrame(std::move(frame));
            context.stats.functionCalls++;

            // Switch to the target program if it's from a library
            if (targetProgram != context.program) {
                context.program = targetProgram;
            }

            // Notify debugger of constructor delegation entry
            if (debugger::DebugHookHelper::isDebuggingEnabled()) {
                auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
                if (sourceLoc) {
                    errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(constructorName, errorsLoc);
                } else {
                    // Fallback: use constructor start location
                    auto ctorStartLoc = context.program->getSourceLocation(funcMetadata->startOffset);
                    if (ctorStartLoc) {
                        errors::SourceLocation errorsLoc(ctorStartLoc->filename, ctorStartLoc->line, ctorStartLoc->column);
                        debugger::DebugHookHelper::enterFunctionHook(constructorName, errorsLoc);
                    } else {
                        debugger::DebugHookHelper::enterFunctionHook(constructorName, errors::SourceLocation());
                    }
                }
            }

            context.instructionPointer = funcMetadata->startOffset - 1;
        } else {
            throw errors::RuntimeException("Constructor '" + constructorName + "' has no bytecode.");
        }
    }

    void ObjectInstanceHelper::handleSuperInvoke(const bytecode::BytecodeProgram::Instruction& instr) {
        // Compiler emits: (classNameIndex, methodNameIndex, argCount)
        // operand[0] = current class name (the class whose method we're executing)
        // operand[1] = method name
        // operand[2] = argument count
        const std::string& currentClassName = context.program->getConstantPool().getString(instr.operands[0]);
        const std::string& methodName = context.program->getConstantPool().getString(instr.operands[1]);
        size_t argCount = instr.operands[2];

        // MYT-196: small-buffer-optimized args buffer.
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i) {
            args[i - 1] = context.stackManager->pop();
        }

        // MYT-208: accept stack-promoted `this`.
        if (context.callStack.empty() || !context.callStack.back().getThisInstanceRaw()) {
            throw errors::RuntimeException("SUPER_INVOKE can only be called from within an instance context");
        }

        auto& currentFrame = context.callStack.back();
        value::Value thisValue = currentFrame.thisInstanceRaw
            ? value::makeStackObjectValue(currentFrame.thisInstanceRaw)
            : value::Value(currentFrame.thisInstance);
        auto* instanceRaw = currentFrame.getThisInstanceRaw();

        // IMPORTANT: Use currentClassName from operand, NOT instance->getClassDefinition()
        // This prevents infinite recursion in multi-level inheritance
        // (e.g., AdvancedService -> DerivedService -> BaseService)
        auto classDef = context.environment->getClassRegistry()->findClass(currentClassName);
        if (!classDef) {
            throw errors::RuntimeException("Current class not found: " + currentClassName);
        }

        if (!classDef->hasParentClass()) {
            throw errors::RuntimeException("Class " + classDef->getName() + " has no parent class");
        }

        std::string parentClassName = classDef->getParentClassName();

        // Extract base parent class name (strip generic type arguments)
        // e.g., "BaseContainer<T>" -> "BaseContainer"
        std::string baseParentClassName = parentClassName;
        size_t genericStart = parentClassName.find('<');
        if (genericStart != std::string::npos) {
            baseParentClassName = parentClassName.substr(0, genericStart);
        }

        auto parentClass = context.environment->getClassRegistry()->findClass(baseParentClassName);
        if (!parentClass) {
            throw errors::RuntimeException("Parent class not found: " + baseParentClassName);
        }

        auto method = parentClass->findMethod(methodName, argCount);
        if (!method) {
            throw errors::RuntimeException("Method not found in parent class " + baseParentClassName + ": " + methodName +
                                         " with " + std::to_string(argCount) + " arguments");
        }

        // Build the mangled name with signature for overloaded methods
        // Use MethodSignature to eliminate manual signature building and 'this' confusion
        auto signature = vm::MethodSignature::fromMethodDefinition(method.get());
        std::string qualifiedName = signature.toMangledName(baseParentClassName, false);  // false = not static

        auto funcMetadata = context.program->getFunction(qualifiedName);
        // Initialize from current call frame's program index (not hardcoded 0)
        // so that if we're already executing in a library, the index is correct
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        // If not found in current program, search loaded library programs
        if (!funcMetadata && context.loadedPrograms) {
            for (size_t i = 0; i < context.loadedPrograms->size(); ++i) {
                auto libFunc = (*context.loadedPrograms)[i]->getFunction(qualifiedName);
                if (libFunc) {
                    funcMetadata = libFunc;
                    targetProgramIndex = i;
                    targetProgram = (*context.loadedPrograms)[i];
                    break;
                }
            }
        }

        if (funcMetadata) {
            size_t frameBase = context.stackManager->size();

            // MYT-208: push receiver Value preserving the storage tag.
            context.stackManager->push(thisValue);

            for (size_t i = 0; i < argCount; ++i) {
                context.stackManager->push(args[i]);
            }

            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = frameBase;
            frame.localBase = frameBase;
            // MYT-197: intern on the target program.
            frame.functionName = targetProgram->internFrameName(qualifiedName);
            // MYT-208: tag-branch `this` ownership for the parent method frame.
            if (value::isStackObject(thisValue))
            {
                frame.thisInstanceRaw = instanceRaw;
            }
            else
            {
                frame.thisInstance = currentFrame.thisInstance;
            }
            frame.definingClassName = baseParentClassName;  // Set parent class as defining class for access control
            frame.programIndex = targetProgramIndex;

            context.pushCallFrame(std::move(frame));
            context.stats.functionCalls++;

            // Switch to the target program if it's from a library
            if (targetProgram != context.program) {
                context.program = targetProgram;
            }

            // Notify debugger of super method entry
            if (debugger::DebugHookHelper::isDebuggingEnabled()) {
                auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
                if (sourceLoc) {
                    errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(qualifiedName, errorsLoc);
                } else {
                    // Fallback: use method start location if current instruction has no location
                    auto methodStartLoc = context.program->getSourceLocation(funcMetadata->startOffset);
                    if (methodStartLoc) {
                        errors::SourceLocation errorsLoc(methodStartLoc->filename, methodStartLoc->line, methodStartLoc->column);
                        debugger::DebugHookHelper::enterFunctionHook(qualifiedName, errorsLoc);
                    } else {
                        debugger::DebugHookHelper::enterFunctionHook(qualifiedName, errors::SourceLocation());
                    }
                }
            }

            context.instructionPointer = funcMetadata->startOffset - 1;
        } else {
            throw errors::RuntimeException("Parent method '" + qualifiedName + "' has no bytecode.");
        }
    }

    void ObjectInstanceHelper::handleSuperGetField(const bytecode::BytecodeProgram::Instruction& instr) {
        // Compiler emits: (classNameIndex, memberNameIndex)
        // operand[0] = current class name (the class whose method we're executing)
        // operand[1] = member/field name
        const std::string& currentClassName = context.program->getConstantPool().getString(instr.operands[0]);
        const std::string& memberName = context.program->getConstantPool().getString(instr.operands[1]);

        // MYT-208: accept stack-promoted `this`.
        if (context.callStack.empty() || !context.callStack.back().getThisInstanceRaw()) {
            throw errors::RuntimeException("SUPER_GET_FIELD can only be called from within an instance context");
        }

        auto* instance = context.callStack.back().getThisInstanceRaw();

        // Get the current class definition
        auto classDef = context.environment->getClassRegistry()->findClass(currentClassName);
        if (!classDef) {
            throw errors::RuntimeException("Current class not found: " + currentClassName);
        }

        if (!classDef->hasParentClass()) {
            throw errors::RuntimeException("Class " + classDef->getName() + " has no parent class");
        }

        // MYT-212: walk parent chain for the first ancestor that declares the
        // field, then read its OWN slot. The previous code called
        // instance->getFieldValue(name), which routes via the instance's
        // runtime class (Child) and so returned the most-derived (shadowed)
        // value — exactly what super.x is supposed to skip.
        auto parent = classDef->getParentClass();
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> owner;
        auto current = parent;
        int depth = 0;
        while (current && depth < 20) {
            if (current->getField(memberName)) {
                owner = current;
                break;
            }
            current = current->getParentClass();
            ++depth;
        }
        if (!owner) {
            throw errors::RuntimeException("Field '" + memberName + "' not found in parent class chain of " + currentClassName);
        }

        size_t slot = owner->getOwnFieldIndex(memberName);
        if (slot == SIZE_MAX) {
            throw errors::RuntimeException("Field '" + memberName + "' has no indexed slot in " + owner->getName());
        }

        context.stackManager->push(instance->getFieldByIndex(slot));
    }

    void ObjectInstanceHelper::handleSuperSetField(const bytecode::BytecodeProgram::Instruction& instr) {
        // Compiler emits: (classNameIndex, memberNameIndex)
        // operand[0] = current class name (the class whose method we're executing)
        // operand[1] = member/field name
        const std::string& currentClassName = context.program->getConstantPool().getString(instr.operands[0]);
        const std::string& memberName = context.program->getConstantPool().getString(instr.operands[1]);

        // MYT-208: accept stack-promoted `this`.
        if (context.callStack.empty() || !context.callStack.back().getThisInstanceRaw()) {
            throw errors::RuntimeException("SUPER_SET_FIELD can only be called from within an instance context");
        }

        auto* instance = context.callStack.back().getThisInstanceRaw();

        // Get the current class definition
        auto classDef = context.environment->getClassRegistry()->findClass(currentClassName);
        if (!classDef) {
            throw errors::RuntimeException("Current class not found: " + currentClassName);
        }

        if (!classDef->hasParentClass()) {
            throw errors::RuntimeException("Class " + classDef->getName() + " has no parent class");
        }

        // Pop the value to assign from the stack
        value::Value assignValue = context.stackManager->pop();

        // MYT-212: write to parent's OWN slot (see handleSuperGetField for rationale).
        auto parent = classDef->getParentClass();
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> owner;
        auto current = parent;
        int depth = 0;
        while (current && depth < 20) {
            if (current->getField(memberName)) {
                owner = current;
                break;
            }
            current = current->getParentClass();
            ++depth;
        }
        if (!owner) {
            throw errors::RuntimeException("Field '" + memberName + "' not found in parent class chain of " + currentClassName);
        }

        size_t slot = owner->getOwnFieldIndex(memberName);
        if (slot == SIZE_MAX) {
            throw errors::RuntimeException("Field '" + memberName + "' has no indexed slot in " + owner->getName());
        }

        instance->setFieldByIndex(slot, assignValue);
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
        auto classRegistry = context.environment->getClassRegistry();
        auto classDef = classRegistry->findClass(baseClassName);

        // Use type-aware constructor lookup for overload resolution
        auto constructor = classDef->findConstructorByTypes(args);
        if (!constructor) {
            bool hasAnyConstructor = !classDef->getConstructors().empty();

            if (argCount == 0 && !hasAnyConstructor) {
                // MYT-208: push the original tagged Value (preserves
                // STACK_OBJECT) instead of constructing an OBJECT Value.
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

        // Build type signature from runtime argument values for overload resolution
        std::string typeSignature = constructor->getTypeSignature();

        std::string constructorName = typeSignature.empty()
            ? baseClassName + "::<init>"
            : baseClassName + "::<init>/" + typeSignature;
        auto funcMetadata = context.program->getFunction(constructorName);
        // Initialize from current call frame's program index (not hardcoded 0)
        // so that if we're already executing in a library, the index is correct
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        // If not found in current program, search loaded library programs
        if (!funcMetadata && context.loadedPrograms) {
            for (size_t i = 0; i < context.loadedPrograms->size(); ++i) {
                auto libFunc = (*context.loadedPrograms)[i]->getFunction(constructorName);
                if (libFunc) {
                    funcMetadata = libFunc;
                    targetProgramIndex = i;
                    targetProgram = (*context.loadedPrograms)[i];
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
        frame.definingClassName = baseClassName;  // Set class as defining class for its own constructor
        frame.programIndex = targetProgramIndex;

        context.pushCallFrame(std::move(frame));
        context.stats.functionCalls++;

        // Switch to the target program if it's from a library
        if (targetProgram != context.program) {
            context.program = targetProgram;
        }

        vm::profiler::ProfilerHookHelper::onFunctionEntry(constructorName);

        // Notify debugger of constructor entry
        if (debugger::DebugHookHelper::isDebuggingEnabled()) {
            auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
            if (sourceLoc) {
                errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                debugger::DebugHookHelper::enterFunctionHook(constructorName, errorsLoc);
            } else {
                // Fallback: use constructor start location if current instruction has no location
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
        if (instr.operands.size() < 2) {
            throw errors::RuntimeException("NEW_OBJECT requires 2 operands: class name index and arg count");
        }

        const std::string& fullClassName = context.program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];


        // Parse generic type arguments and extract base class name
        std::unordered_map<std::string, std::string> genericTypeBindings;
        std::string baseClassName = parseGenericTypeArguments(fullClassName, genericTypeBindings);

        // MYT-196: pop constructor arguments into a small-buffer-optimized scratch.
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i) {
            args[i - 1] = context.stackManager->pop();
        }

        // PHASE 2 OPTIMIZATION: Integer Caching
        // If creating Int object with single int argument in cacheable range, use cached instance
        if (baseClassName == "Int" && argCount == 1 && value::isInt(args[0])) {
            int intValue = static_cast<int>(value::asInt(args[0]));

            // Check if value is cacheable
            if (value::IntegerCache::isCacheable(intValue)) {
                // Get Int class definition for cache
                auto classRegistry = context.environment->getClassRegistry();
                auto intClassDef = classRegistry ? classRegistry->findClass("Int") : nullptr;

                if (intClassDef) {
                    // Try to get from cache
                    auto cachedInstance = value::IntegerCache::getInt(intValue, intClassDef);

                    if (cachedInstance) {
                        // Cache hit! Return cached Int object (already initialized)
                        // Skip constructor invocation - cached object is already properly initialized
                        // MYT-208: invokeConstructor takes a Value receiver.
                        invokeConstructor(value::Value(cachedInstance), baseClassName, args.span());
                        return;
                    }
                }
            }
        }

        // Bool caching: only two values, so cache hit rate is 100%.
        if (baseClassName == "Bool" && argCount == 1 && value::isBool(args[0])) {
            auto classRegistry = context.environment->getClassRegistry();
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
                auto classRegistry = context.environment->getClassRegistry();
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
        // intern range is cached up to StringCache::kMaxEntries with FIFO
        // eviction.
        if (baseClassName == "String" && argCount == 1) {
            std::string strValue;
            bool gotString = false;
            if (value::isString(args[0])) {
                strValue = value::asString(args[0]);
                gotString = true;
            } else if (value::isInternedString(args[0])) {
                strValue = value::asInternedString(args[0]).getString();
                gotString = true;
            }
            if (gotString) {
                auto classRegistry = context.environment->getClassRegistry();
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

        // Normal object creation path (non-cached or cache miss)
        auto instance = createObjectInstance(baseClassName, genericTypeBindings);
        attachSpecializedCollectionIfNeeded(
            instance.get(),
            baseClassName,
            genericTypeBindings,
            collectionInitialCapacity,
            context.environment->getClassRegistry());

        // Invoke constructor using the class definition's actual name
        // (handles aliases: "MyInt" resolves to same ClassDef as "Int",
        //  but constructor bytecode is registered under "Int::<init>")
        std::string actualClassName = instance->getClassDefinition()->getName();
        // MYT-208: NEW_OBJECT path always produces an OBJECT-tagged Value.
        invokeConstructor(value::Value(instance), actualClassName, args.span());
    }

    void ObjectInstanceHelper::handleNewStack(const bytecode::BytecodeProgram::Instruction& instr) {
        if (instr.operands.size() < 2) {
            throw errors::RuntimeException("NEW_STACK requires 2 operands: class name index and arg count");
        }

        const std::string& fullClassName = context.program->getConstantPool().getString(instr.operands[0]);
        size_t argCount = instr.operands[1];

        std::unordered_map<std::string, std::string> genericTypeBindings;
        std::string baseClassName = parseGenericTypeArguments(fullClassName, genericTypeBindings);

        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i) {
            args[i - 1] = context.stackManager->pop();
        }

        auto classRegistry = context.environment->getClassRegistry();
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

    std::string ObjectInstanceHelper::getCurrentClassName() {
        if (!context.callStack.empty()) {
            // MYT-208: accept stack-promoted `this`.
            if (auto* rawThis = context.callStack.back().getThisInstanceRaw()) {
                return rawThis->getClassDefinition()->getName();
            }
            // MYT-197: prefer frame.definingClassName (populated at push time)
            // over resolving the interned handle and re-splitting.
            if (!context.callStack.back().definingClassName.empty()) {
                return context.callStack.back().definingClassName;
            }
            const std::string& funcName = context.frameName(context.callStack.back());
            size_t colonPos = funcName.find("::");
            if (colonPos != std::string::npos) {
                std::string className = funcName.substr(0, colonPos);
                return className;
            }
        }
        return "";
    }

    bool ObjectInstanceHelper::isSubclass(const std::string& derivedClass, const std::string& baseClass) {
        if (derivedClass.empty()) return false;
        auto currentClass = context.environment->getClassRegistry()->findClass(derivedClass);
        while (currentClass && currentClass->hasParentClass()) {
            std::string parentClassName = currentClass->getParentClassName();

            // Extract base parent class name (strip generic type arguments)
            std::string baseParentClassName = parentClassName;
            size_t genericStart = parentClassName.find('<');
            if (genericStart != std::string::npos) {
                baseParentClassName = parentClassName.substr(0, genericStart);
            }

            if (baseParentClassName == baseClass) {
                return true;
            }
            auto parentClass = context.environment->getClassRegistry()->findClass(baseParentClassName);
            currentClass = parentClass;
        }
        return false;
    }

    validation::AccessContext ObjectInstanceHelper::createAccessContext(
        const std::string& targetClassName,
        bool isSetter)
    {
        std::string currentClassName = getCurrentClassName();
        bool isSameClass = (currentClassName == targetClassName);
        bool isSubclassCheck = isSubclass(currentClassName, targetClassName);

        // Special case: Static field initialization (SET operations) happens in global scope
        // Allow SET during static initialization
        if (currentClassName.empty() && context.callStack.empty() && isSetter) {
            // Allow initialization by treating it as same class access
            isSameClass = true;
        }

        return validation::AccessContext(
            currentClassName,
            targetClassName,
            isSameClass,
            isSubclassCheck,
            isSetter,
            errors::SourceLocation()
        );
    }
}
