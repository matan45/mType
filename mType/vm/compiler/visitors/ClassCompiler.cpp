#include "ClassCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "../analysis/DefiniteAssignmentAnalyzer.hpp"
#include "../../bytecode/OpCode.hpp"
#include <unordered_set>

namespace vm::compiler::visitors
{
    ClassCompiler::ClassCompiler(CompilerContext& context)
        : ctx(context)
        , methodHelper(std::make_unique<MethodCompilerHelper>(context))
        , paramValidator(std::make_unique<ParameterValidator>(context))
        , overloadResolver(std::make_unique<overload::OverloadResolutionHelper>(context))
    {
    }

    value::Value ClassCompiler::compileClass(ast::ClassNode* node)
    {
        auto previousClassNode = ctx.currentClassNode;
        ctx.currentClassNode = node;

        for (const auto& method : node->getMethods())
        {
            method->accept(ctx.visitor);
        }

        for (const auto& constructor : node->getConstructors())
        {
            constructor->accept(ctx.visitor);
        }

        if (node->getConstructors().empty())
        {
            methodHelper->compileDefaultConstructor(node);
        }

        compileStaticInitializer(node);

        // Phase 2 (allocation perf): record own-class instance fields that every
        // constructor definitely assigns before any read. initializeObjectFields()
        // uses this to skip redundant default-init setField() calls.
        computeSkipDefaultInitFields(node);

        // Phase 3 (allocation perf): flag constructors whose body is strictly
        // `this.F_k = param_k`. The VM's createObject / invokeConstructor
        // fast-paths copy args straight into instance fields, skipping the
        // whole CallFrame push + ctor-bytecode interpret loop.
        markTrivialConstructors(node);

        ctx.currentClassNode = previousClassNode;

        return std::monostate{};
    }

    void ClassCompiler::compileStaticInitializer(ast::ClassNode* node)
    {
        bool hasStaticInitializers = false;
        for (const auto& fieldPtr : node->getFields())
        {
            auto* fieldNode = dynamic_cast<ast::FieldNode*>(fieldPtr.get());
            if (fieldNode && fieldNode->getIsStatic() && fieldNode->hasInitialValue())
            {
                hasStaticInitializers = true;
                break;
            }
        }

        if (!hasStaticInitializers)
        {
            return;
        }

        const std::string className = node->getClassName();
        const std::string initializerName = className + "::<static_init>$static";

        size_t skipJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP, node);
        size_t initializerStart = ctx.program.getCurrentOffset();

        bytecode::BytecodeProgram::FunctionMetadata tempMetadata;
        tempMetadata.name = initializerName;
        tempMetadata.startOffset = initializerStart;
        tempMetadata.instructionCount = 0;
        tempMetadata.localCount = 0;
        tempMetadata.parameterCount = 0;
        tempMetadata.returnType = "void";
        tempMetadata.isStatic = true;
        tempMetadata.isNative = false;
        ctx.program.registerFunction(initializerName, tempMetadata);

        ctx.functionFrameManager.enterFunctionFrame(initializerName,
                                                    "void",
                                                    ctx.variableTracker.getNextLocalSlot(),
                                                    ctx.variableTracker.getCurrentScopeDepth(),
                                                    false,
                                                    false);
        ctx.variableTracker.beginScope();

        const bool previousStaticInitializer = ctx.inStaticFieldInitializer;
        ctx.inStaticFieldInitializer = true;

        for (const auto& fieldPtr : node->getFields())
        {
            auto* fieldNode = dynamic_cast<ast::FieldNode*>(fieldPtr.get());
            if (!fieldNode || !fieldNode->getIsStatic() || !fieldNode->hasInitialValue())
            {
                continue;
            }

            fieldNode->getInitialValue()->accept(ctx.visitor);

            std::string qualifiedName = className + "::" + fieldNode->getName();
            size_t nameIndex = ctx.program.getConstantPool().addString(qualifiedName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_STATIC,
                                         static_cast<uint64_t>(nameIndex),
                                         fieldNode);
        }

        ctx.inStaticFieldInitializer = previousStaticInitializer;

        ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
        ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);

        size_t localCount = ctx.functionFrameManager.getLocalCount();
        ctx.variableTracker.endScope();
        ctx.functionFrameManager.exitFunctionFrame();

        size_t initializerEnd = ctx.program.getCurrentOffset();
        ctx.emitter.patchJump(skipJump);

        size_t initializerNameIndex = ctx.program.getConstantPool().addString(initializerName);
        ctx.emitter.emitWithLocation(bytecode::OpCode::CALL,
                                     static_cast<uint64_t>(initializerNameIndex),
                                     0u,
                                     node);
        ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = initializerName;
        metadata.startOffset = initializerStart;
        metadata.instructionCount = initializerEnd - initializerStart;
        metadata.localCount = localCount;
        metadata.parameterCount = 0;
        metadata.returnType = "void";
        metadata.isStatic = true;
        metadata.isNative = false;

        if (const auto* existingMetadata = ctx.program.getFunction(initializerName))
        {
            metadata.exceptionTable = existingMetadata->exceptionTable;
        }

        ctx.program.registerFunction(initializerName, metadata);
        ctx.program.addStaticInitializerFunction(initializerName);
    }

    void ClassCompiler::markTrivialConstructors(ast::ClassNode* node)
    {
        auto classDef = ctx.env->findClass(node->getClassName());
        if (!classDef) return;

        const auto& ctorNodes = node->getConstructors();
        const auto& ctorDefs = classDef->getConstructors();

        // Order must line up — ClassRegistrar adds each ctor in AST order. If they
        // don't match (e.g., an implicit default ctor snuck in), bail rather than guess.
        if (ctorNodes.size() != ctorDefs.size()) return;

        for (size_t i = 0; i < ctorNodes.size(); ++i)
        {
            auto* ctor = dynamic_cast<ast::nodes::classes::ConstructorNode*>(ctorNodes[i].get());
            if (!ctor) continue;
            if (ctor->hasSuperInitializer()) continue;

            std::vector<std::string> paramNames;
            const auto& params = ctor->getParametersWithTypes();
            paramNames.reserve(params.size());
            for (const auto& p : params) paramNames.push_back(p.first);

            auto trivial = analysis::DefiniteAssignmentAnalyzer::analyzeTrivialCtor(
                ctor->getBodyPtr(), paramNames);
            if (!trivial.has_value()) continue;

            // Keep only assignments to this-class instance fields. Inherited fields
            // could legitimately live on the slow path; we don't silently skip
            // parent-side initialiser bytecode.
            const auto& ownInstanceFields = classDef->getInstanceFields();
            std::vector<std::pair<std::string, size_t>> filtered;
            std::vector<std::pair<size_t, size_t>> indexed;
            filtered.reserve(trivial->size());
            indexed.reserve(trivial->size());
            bool allOwned = true;
            for (const auto& pr : *trivial)
            {
                if (ownInstanceFields.count(pr.first) == 0) { allOwned = false; break; }
                size_t fieldIdx = classDef->getFieldIndex(pr.first);
                if (fieldIdx == SIZE_MAX) { allOwned = false; break; }
                filtered.push_back(pr);
                indexed.emplace_back(fieldIdx, pr.second);
            }
            if (!allOwned) continue;

            // Final instance fields with inline initializers (e.g. `final int CHANNELS
            // = 32`) are set by the constructor's bytecode prologue. The trivial fast
            // path skips the entire constructor bytecode, so those initialisations
            // would be lost. Reject trivial classification when any final instance
            // field exists in this class or its hierarchy.
            bool hasFinalFields = false;
            for (const auto& [fname, fdef] : ownInstanceFields)
            {
                if (fdef->isFinal()) { hasFinalFields = true; break; }
            }
            if (!hasFinalFields)
            {
                auto parent = classDef->getParentClass();
                while (parent && !hasFinalFields)
                {
                    for (const auto& [fname, fdef] : parent->getInstanceFields())
                    {
                        if (fdef->isFinal()) { hasFinalFields = true; break; }
                    }
                    parent = parent->getParentClass();
                }
            }
            if (hasFinalFields) continue;

            ctorDefs[i]->setTrivialFieldAssignments(std::move(filtered), std::move(indexed));
        }
    }

    void ClassCompiler::computeSkipDefaultInitFields(ast::ClassNode* node)
    {
        auto classDef = ctx.env->findClass(node->getClassName());
        if (!classDef) return;

        const auto& ctorNodes = node->getConstructors();
        if (ctorNodes.empty()) return;  // implicit default ctor touches nothing — leave empty

        std::unordered_set<std::string> intersection;
        bool first = true;

        for (const auto& ctorNode : ctorNodes)
        {
            auto* ctor = dynamic_cast<ast::nodes::classes::ConstructorNode*>(ctorNode.get());
            if (!ctor)
            {
                intersection.clear();
                break;
            }

            // A super() / this() call running before body statements could read
            // this.X via super's own constructor. The analyser rejects any
            // non-trivial shape, but being explicit here keeps future readers honest.
            if (ctor->hasSuperInitializer())
            {
                intersection.clear();
                break;
            }

            auto assigned = analysis::DefiniteAssignmentAnalyzer::analyzeConstructor(ctor->getBodyPtr());
            if (assigned.empty())
            {
                intersection.clear();
                break;
            }

            if (first)
            {
                intersection = std::move(assigned);
                first = false;
            }
            else
            {
                std::unordered_set<std::string> next;
                for (const auto& name : intersection)
                {
                    if (assigned.count(name) > 0) next.insert(name);
                }
                intersection = std::move(next);
                if (intersection.empty()) break;
            }
        }

        // Only keep fields that belong to *this* class (not inherited). Parent
        // classes handle their own fields via their own initializeObjectFields pass.
        std::unordered_set<std::string> ownFieldsOnly;
        const auto& ownInstanceFields = classDef->getInstanceFields();
        for (const auto& name : intersection)
        {
            if (ownInstanceFields.count(name) > 0) ownFieldsOnly.insert(name);
        }

        classDef->setSkipDefaultInitFields(std::move(ownFieldsOnly));
    }

    value::Value ClassCompiler::compileMethod(ast::MethodNode* node)
    {
        return methodHelper->compileMethod(node);
    }

    value::Value ClassCompiler::compileConstructor(ast::ConstructorNode* node)
    {
        return methodHelper->compileConstructor(node);
    }

    value::Value ClassCompiler::compileField(ast::FieldNode* node)
    {
        // Instance fields are initialized in constructors; only static fields land here.
        if (node->getIsStatic() && node->hasInitialValue())
        {
            node->getInitialValue()->accept(ctx.visitor);

            std::string className = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
            std::string qualifiedName = className + "::" + node->getName();
            size_t nameIndex = ctx.program.getConstantPool().addString(qualifiedName);

            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_STATIC, static_cast<uint64_t>(nameIndex), node);
        }

        return std::monostate{};
    }

    bool ClassCompiler::tryAutoBoxArgument(ast::ASTNode* argument, const std::string& expectedType)
    {
        bool isBoxType = (expectedType == "Int" ||
                          expectedType == "Float" ||
                          expectedType == "Bool" ||
                          expectedType == "String");

        if (!isBoxType || !argument)
        {
            return false;
        }

        value::ValueType argType = ctx.typeInference.inferExpressionType(argument);
        bool needsBoxing = false;

        if (expectedType == "Int" && argType == value::ValueType::INT) needsBoxing = true;
        else if (expectedType == "Float" && argType == value::ValueType::FLOAT) needsBoxing = true;
        else if (expectedType == "Bool" && argType == value::ValueType::BOOL) needsBoxing = true;
        else if (expectedType == "String" && argType == value::ValueType::STRING) needsBoxing = true;

        if (!needsBoxing)
        {
            return false;
        }

        argument->accept(ctx.visitor);

        size_t classNameIndex = ctx.program.getConstantPool().addString(expectedType);
        auto boxClassDef = ctx.env->findClass(expectedType);
        bool boxIsValue = boxClassDef && boxClassDef->isValueClass();
        if (boxIsValue) {
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_VALUE_OBJECT,
                                         static_cast<uint64_t>(classNameIndex),
                                         1u, argument);
            ctx.emitter.emitWithLocation(bytecode::OpCode::OBJECT_TO_VALUE, argument);
        } else {
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                         static_cast<uint64_t>(classNameIndex),
                                         1u, argument);
        }

        return true;
    }

    bool ClassCompiler::isReceiverNonNullable(ast::ASTNode* receiverNode)
    {
        return !ctx.typeInference.inferExpressionNullable(receiverNode);
    }
}
