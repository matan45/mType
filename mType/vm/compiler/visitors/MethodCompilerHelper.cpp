#include "MethodCompilerHelper.hpp"
#include <cstddef>
#include <cstdint>
#include "../../bytecode/OpCode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../MethodSignature.hpp"

namespace vm::compiler::visitors
{
    MethodCompilerHelper::MethodCompilerHelper(CompilerContext& context)
        : ctx(context)
    {
    }

    void MethodCompilerHelper::compileDefaultConstructor(ast::ClassNode* node)
    {
        size_t skipJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP, node);
        size_t constructorStart = ctx.program.getCurrentOffset();

        std::vector<std::string> paramNames = {"this"};

        std::string className = node->getClassName();
        std::string constructorName = className + "::<init>";

        // Pre-register so exception tables can be added during initialization.
        bytecode::BytecodeProgram::FunctionMetadata tempMetadata;
        tempMetadata.name = constructorName;
        tempMetadata.startOffset = constructorStart;
        tempMetadata.instructionCount = 0;
        tempMetadata.localCount = 0;
        tempMetadata.parameterCount = 1;
        tempMetadata.returnType = "object";
        tempMetadata.isStatic = false;
        tempMetadata.isNative = false;
        ctx.program.registerFunction(constructorName, tempMetadata);

        // MYT-112: isConstructor=true allows bare `return;`.
        ctx.functionFrameManager.enterFunctionFrame(constructorName,
                                                    "object",
                                                    ctx.variableTracker.getNextLocalSlot(),
                                                    ctx.variableTracker.getCurrentScopeDepth(),
                                                    false,
                                                    false,
                                                    true);
        ctx.variableTracker.beginScope();

        ctx.variableTracker.declareLocal("this", value::ValueType::OBJECT,
                                         node->getClassName());

        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

        if (node->hasParentClass())
        {
            std::string parentClassName = node->getParentClassName();
            auto parentClassDef = ctx.env->getClassRegistry()->findClass(parentClassName);
            if (parentClassDef)
            {
                auto parentDefaultCtor = parentClassDef->findConstructor(0);
                if (parentDefaultCtor)
                {
                    std::string currentClassName = node->getClassName();
                    size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::SUPER_CONSTRUCTOR,
                                     static_cast<uint64_t>(classNameIndex),
                                     static_cast<uint64_t>(0), node);
                }
            }
        }

        initializeInstanceFields(node);

        ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
        ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);

        size_t localCount = ctx.functionFrameManager.getLocalCount();

        const auto& locals = ctx.variableTracker.getLocals();
        const auto& currentFrame = ctx.functionFrameManager.currentFrame();
        std::vector<std::string> localVarNames(localCount);
        for (const auto& local : locals)
        {
            if (local.slot >= currentFrame.localStartSlot)
            {
                size_t relativeSlot = local.slot - currentFrame.localStartSlot;
                if (relativeSlot < localCount)
                {
                    localVarNames[relativeSlot] = local.name;
                }
            }
        }

        ctx.variableTracker.endScope();
        ctx.functionFrameManager.exitFunctionFrame();

        size_t constructorEnd = ctx.program.getCurrentOffset();
        ctx.emitter.patchJump(skipJump);

        // Preserve exception table built during initialization.
        auto* existingMetadata = const_cast<bytecode::BytecodeProgram::FunctionMetadata*>(
            ctx.program.getFunction(constructorName)
        );

        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = constructorName;
        metadata.startOffset = constructorStart;
        metadata.instructionCount = constructorEnd - constructorStart;
        metadata.localCount = localCount;
        metadata.parameterCount = 1;
        metadata.parameterNames = paramNames;
        metadata.returnType = "object";
        metadata.isStatic = false;
        metadata.isNative = false;
        metadata.localVariableNames = localVarNames;

        if (existingMetadata) {
            metadata.exceptionTable = existingMetadata->exceptionTable;
        }

        ctx.program.registerFunction(metadata.name, metadata);

        if (node->isGeneric())
        {
            std::string fullClassName = node->getFullClassName();
            std::string genericConstructorName = fullClassName + "::<init>";
            metadata.name = genericConstructorName;
            ctx.program.registerFunction(genericConstructorName, metadata);
        }
    }

    void MethodCompilerHelper::initializeInstanceFields(ast::ClassNode* node)
    {
        // MYT-212: emit class-targeted SET_FIELD_TYPED so the inline
        // initializer writes to THIS class's own slot, not the most-derived
        // slot of the runtime instance. Without this, when Child shadows
        // Parent's `x`, Parent's initializer (running during construction of
        // a Child instance) would land in Child's slot, leaving Parent's
        // slot uninitialised — and `super.x` would read 0.
        const std::string& enclosingClassName = node->getClassName();
        size_t classNameIndex = ctx.program.getConstantPool().addString(enclosingClassName);

        auto& fields = node->getFields();
        for (const auto& fieldPtr : fields)
        {
            if (auto* fieldNode = dynamic_cast<ast::FieldNode*>(fieldPtr.get()))
            {
                if (!fieldNode->getIsStatic() && fieldNode->getInitialValue())
                {
                    ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, fieldNode);

                    fieldNode->getInitialValue()->accept(ctx.visitor);

                    std::string fieldName = fieldNode->getName();
                    size_t fieldNameIndex = ctx.program.getConstantPool().addString(fieldName);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD_TYPED,
                        std::vector<uint64_t>{
                            static_cast<uint64_t>(classNameIndex),
                            static_cast<uint64_t>(fieldNameIndex)
                        }, fieldNode);

                    // SET_FIELD_TYPED leaves the assigned value on the stack
                    // (mirroring SET_FIELD for chained-assignment expressions);
                    // pop it so the ctor body sees a clean stack.
                    ctx.emitter.emitWithLocation(bytecode::OpCode::POP, fieldNode);
                }
            }
        }
    }

    value::Value MethodCompilerHelper::compileMethod(ast::MethodNode* node)
    {
        bool isStatic = node->getIsStatic();

        bool wasInInstanceMethod = ctx.inInstanceMethod;
        bool wasInStaticMethod = ctx.inStaticMethod;
        ast::MethodNode* wasMethodNode = ctx.currentMethodNode;
        ctx.inInstanceMethod = !isStatic;
        ctx.inStaticMethod = isStatic;
        ctx.currentMethodNode = node;

        ctx.typeInference.setCurrentClassContext(ctx.currentClassNode, ctx.inInstanceMethod);

        if (node->isGeneric())
        {
            const auto& genericParams = node->getGenericTypeParameters();

            // Method-level generic parameters must not shadow class-level ones.
            if (ctx.currentClassNode && ctx.currentClassNode->isGeneric())
            {
                const auto& classGenericParams = ctx.currentClassNode->getGenericParameters();
                for (const auto& methodParam : genericParams)
                {
                    for (const auto& classParam : classGenericParams)
                    {
                        if (methodParam.name == classParam.name)
                        {
                            throw errors::TypeException(
                                "Method generic type parameter '" + methodParam.name +
                                "' shadows class-level type parameter with the same name. " +
                                "Use a different name to avoid confusion.",
                                methodParam.location
                            );
                        }
                    }
                }
            }

            for (const auto& param : genericParams)
            {
                ctx.program.getConstantPool().addString(param.name);
            }
        }

        size_t skipJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP, node);
        size_t methodStart = ctx.program.getCurrentOffset();

        // MYT-161: PROFILE_ENTER as method's first instruction, mirroring
        // FunctionCompiler.cpp for standalone functions. Without this, methods
        // are never profiled hot, jitCompiler->compile is never invoked,
        // jitCodeCache stays empty, and CALL_METHOD's IC direct-JIT dispatch
        // can never fire. JIT treats PROFILE_ENTER as a no-op so it's free.
        ctx.program.emit(bytecode::OpCode::PROFILE_ENTER);

        MethodParameters params = collectMethodParameters(node, isStatic);

        // Pre-register so exception tables can be added during body compilation.
        // MYT-279: route through MethodSignature so the mangled name matches
        // both finalizeMethodCompilation's overwrite and ClassRegistrar's
        // pre-registration. Skip "this" for instance methods.
        std::string qualifiedMethodName = node->getName();
        if (ctx.currentClassNode)
        {
            std::vector<std::string> sigTypes(
                params.paramTypes.begin() + (isStatic ? 0 : 1),
                params.paramTypes.end());
            qualifiedMethodName = vm::MethodSignature(node->getName(), sigTypes)
                .toMangledName(ctx.currentClassNode->getClassName(), isStatic);
        }

        bytecode::BytecodeProgram::FunctionMetadata tempMetadata;
        tempMetadata.name = qualifiedMethodName;
        tempMetadata.startOffset = methodStart;
        tempMetadata.instructionCount = 0;
        tempMetadata.localCount = 0;
        tempMetadata.parameterCount = params.paramNames.size();
        tempMetadata.parameterNames = params.paramNames;
        tempMetadata.parameterTypes = params.paramTypes;
        tempMetadata.parameterNullable = params.paramNullable;
        tempMetadata.returnType = params.returnTypeStr;
        tempMetadata.isAsync = node->getIsAsync();
        tempMetadata.isStatic = isStatic;
        tempMetadata.isNative = false;

        ctx.program.registerFunction(qualifiedMethodName, tempMetadata);

        MethodBodyInfo bodyInfo = compileMethodBodyWithFrame(node, params, isStatic, qualifiedMethodName);

        ctx.inInstanceMethod = wasInInstanceMethod;
        ctx.inStaticMethod = wasInStaticMethod;
        ctx.currentMethodNode = wasMethodNode;

        ctx.typeInference.setCurrentClassContext(ctx.currentClassNode, ctx.inInstanceMethod);

        finalizeMethodCompilation(node, params, methodStart, skipJump, bodyInfo, isStatic);

        return std::monostate{};
    }

    value::Value MethodCompilerHelper::compileConstructor(ast::ConstructorNode* node)
    {
        const auto& params = node->getParametersWithTypes();

        bool wasInInstanceMethod = ctx.inInstanceMethod;
        ctx.inInstanceMethod = true;

        ctx.typeInference.setCurrentClassContext(ctx.currentClassNode, ctx.inInstanceMethod);

        size_t skipJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP, node);

        size_t constructorStart = ctx.program.getCurrentOffset();

        std::vector<std::string> paramNames;
        paramNames.push_back("this");
        for (const auto& param : params)
        {
            paramNames.push_back(param.first);
        }

        std::string returnTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(value::ValueType::OBJECT);

        std::string className = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";

        // MYT-282: ParameterType overload preserves `int[]` etc. for ARRAY-tag
        // parameters in constructor overload-resolution keys.
        std::string typeSignature;
        for (size_t i = 0; i < params.size(); ++i) {
            if (i > 0) typeSignature += ",";
            typeSignature += ::types::TypeConversionUtils::getTypeDisplayName(params[i].second);
        }

        std::string constructorName;
        if (typeSignature.empty()) {
            constructorName = className + "::<init>";
        } else {
            constructorName = className + "::<init>/" + typeSignature;
        }

        bytecode::BytecodeProgram::FunctionMetadata tempMetadata;
        tempMetadata.name = constructorName;
        tempMetadata.startOffset = constructorStart;
        tempMetadata.instructionCount = 0;
        tempMetadata.localCount = 0;
        tempMetadata.parameterCount = paramNames.size();
        tempMetadata.parameterNames = paramNames;
        tempMetadata.returnType = returnTypeStr;
        tempMetadata.isStatic = false;
        tempMetadata.isNative = false;
        ctx.program.registerFunction(constructorName, tempMetadata);

        // MYT-112: isConstructor=true allows bare `return;` inside the body.
        ctx.functionFrameManager.enterFunctionFrame(constructorName,
                                                    returnTypeStr,
                                                    ctx.variableTracker.getNextLocalSlot(),
                                                    ctx.variableTracker.getCurrentScopeDepth(),
                                                    false,
                                                    false,
                                                    true);
        ctx.variableTracker.beginScope();

        for (const auto& paramName : paramNames)
        {
            ctx.variableTracker.declareLocal(paramName, value::ValueType::VOID, "");
        }

        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

        // Assign constructor parameters to matching instance fields BEFORE
        // super() so polymorphic methods called in parent constructors see
        // correct field values. Final fields are skipped — they must be
        // assigned exactly once, either via inline initializer or explicit
        // `this.X = ...`; auto-binding them would be the *first* write,
        // tripping the instance-final check on the user's explicit assignment.
        if (ctx.currentClassNode)
        {
            const auto& fields = ctx.currentClassNode->getFields();
            for (size_t i = 0; i < params.size(); ++i)
            {
                const std::string& paramName = params[i].first;

                for (const auto& field : fields)
                {
                    auto* fieldNode = dynamic_cast<ast::FieldNode*>(field.get());
                    if (fieldNode && !fieldNode->getIsStatic() && !fieldNode->getIsFinal()
                        && fieldNode->getName() == paramName)
                    {
                        ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);

                        ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL,
                                                   static_cast<uint64_t>(i + 1), node);

                        size_t fieldNameIndex = ctx.program.getConstantPool().addString(paramName);
                        ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD,
                                                   static_cast<uint64_t>(fieldNameIndex), node);
                        break;
                    }
                }
            }
        }

        if (ctx.currentClassNode && ctx.currentClassNode->hasParentClass())
        {
            std::string parentClassName = ctx.currentClassNode->getParentClassName();
            auto parentClassDef = ctx.env->getClassRegistry()->findClass(parentClassName);

            if (node->hasSuperInitializer())
            {
                auto* superInit = node->getSuperInitializer();
                if (superInit)
                {
                    superInit->accept(ctx.visitor);
                }
            }
            else if (parentClassDef)
            {
                auto parentDefaultCtor = parentClassDef->findConstructor(0);
                if (parentDefaultCtor)
                {
                    std::string currentClassName = ctx.currentClassNode->getClassName();
                    size_t classNameIndex = ctx.program.getConstantPool().addString(currentClassName);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::SUPER_CONSTRUCTOR,
                                     static_cast<uint64_t>(classNameIndex),
                                     static_cast<uint64_t>(0), node);
                }
            }
        }

        if (ctx.currentClassNode)
        {
            initializeInstanceFields(ctx.currentClassNode);
        }

        auto* body = node->getBodyPtr();
        if (body)
        {
            body->accept(ctx.visitor);
        }

        ctx.inInstanceMethod = wasInInstanceMethod;

        ctx.typeInference.setCurrentClassContext(ctx.currentClassNode, ctx.inInstanceMethod);

        // Constructors implicitly return 'this'.
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
        ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);

        size_t localCount = ctx.functionFrameManager.getLocalCount();

        const auto& locals = ctx.variableTracker.getLocals();
        const auto& currentFrame = ctx.functionFrameManager.currentFrame();
        std::vector<std::string> localVarNames(localCount);
        for (const auto& local : locals)
        {
            if (local.slot >= currentFrame.localStartSlot)
            {
                size_t relativeSlot = local.slot - currentFrame.localStartSlot;
                if (relativeSlot < localCount)
                {
                    localVarNames[relativeSlot] = local.name;
                }
            }
        }

        ctx.variableTracker.endScope();
        ctx.functionFrameManager.exitFunctionFrame();

        size_t constructorEnd = ctx.program.getCurrentOffset();

        ctx.emitter.patchJump(skipJump);

        // Preserve exception table built during body compilation.
        auto* existingMetadata = const_cast<bytecode::BytecodeProgram::FunctionMetadata*>(
            ctx.program.getFunction(constructorName)
        );

        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = constructorName;
        metadata.startOffset = constructorStart;
        metadata.instructionCount = constructorEnd - constructorStart;
        metadata.localCount = localCount;
        metadata.parameterCount = paramNames.size();
        metadata.parameterNames = paramNames;
        metadata.returnType = returnTypeStr;
        metadata.isStatic = false;
        metadata.localVariableNames = localVarNames;
        metadata.isNative = false;

        if (existingMetadata) {
            metadata.exceptionTable = existingMetadata->exceptionTable;
        }

        ctx.program.registerFunction(constructorName, metadata);

        if (ctx.currentClassNode && ctx.currentClassNode->isGeneric())
        {
            std::string fullClassName = ctx.currentClassNode->getFullClassName();
            std::string genericConstructorName = fullClassName + "::<init>/" + typeSignature;
            ctx.program.registerFunction(genericConstructorName, metadata);
        }

        return std::monostate{};
    }
}
