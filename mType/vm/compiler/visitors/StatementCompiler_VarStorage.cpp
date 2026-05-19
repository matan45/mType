#include "StatementCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/EnvironmentException.hpp"
#include "../../../errors/UndefinedException.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"

namespace vm::compiler::visitors
{
    void StatementCompiler::validateReassignmentType(ast::AssignmentNode* node, const std::string& existingClassName)
    {
        auto* value = node->getValue();
        if (existingClassName.empty() || !value)
        {
            return;
        }

        std::string valueClassName = ctx.typeInference.inferExpressionClassName(value);
        value::ValueType valueType = ctx.typeInference.inferExpressionType(value);
        bool isNullValue = ctx.typeInference.isEffectivelyNullLiteral(value);

        bool existingIsNullable = true;
        std::string varName = node->getVariableName();
        if (ctx.functionFrameManager.isInFunction())
        {
            existingIsNullable = ctx.variableTracker.getLocalNullableByName(varName);
        }
        else if (ctx.globalRegistry.exists(varName))
        {
            existingIsNullable = ctx.globalRegistry.isNullable(varName);
        }

        if (!valueClassName.empty() || valueType != value::ValueType::OBJECT)
        {
            std::string resolvedExistingClassName = ctx.resolveGenericType(existingClassName);
            std::string resolvedValueClassName = ctx.resolveGenericType(valueClassName);

            ctx.typeValidator.validateAssignment(value::ValueType::OBJECT, resolvedExistingClassName,
                                                 valueType, resolvedValueClassName, isNullValue, node->getLocation(),
                                                 existingIsNullable);
        }
    }

    void StatementCompiler::emitVariableDeclaration(ast::AssignmentNode* node)
    {
        std::string name = node->getVariableName();
        value::ValueType varType = node->getVariableType();

        // Top-level pseudo-frame has scopeDepthStart=0 and localStartSlot=0.
        // Variables in anonymous blocks (scope depth > 2) should be locals
        // even at global scope, so they can be captured by lambdas.
        bool isInRealFunction = false;
        try {
            if (ctx.functionFrameManager.isInFunction()) {
                const auto& frame = ctx.functionFrameManager.currentFrame();

                if (frame.scopeDepthStart == 0 && frame.localStartSlot == 0) {
                    int depth = ctx.variableTracker.getCurrentScopeDepth();
                    if (depth > 3) {
                        isInRealFunction = true;
                    }
                    // MYT-XXX (top-level decl promotion): module-level decls
                    // (scope depth 2–3) are normally globals (DECLARE_VAR),
                    // but we promote them to entry-frame locals (STORE_LOCAL)
                    // when it's safe — eliminating LOAD_VAR/STORE_VAR helper
                    // calls in tight loops and unlocking non-boxed-mode JIT.
                    // Preconditions:
                    //   1. Not inside an imported file.
                    //   2. Not declared `final` — finality is enforced via the
                    //      DECLARE_VAR operand; locals don't track it.
                    //   3. NestedReferenceCollector did not bail out.
                    //   4. Name is not referenced from any nested callable body.
                    else if (!ctx.inImportedFile &&
                             !node->getIsFinal() &&
                             !ctx.nestedReferencesPessimistic &&
                             !ctx.namesReferencedByNestedNonLambdaFns.count(name))
                    {
                        isInRealFunction = true;
                    }
                    else {
                        isInRealFunction = false;
                    }
                } else {
                    isInRealFunction = true;
                }
            }
        } catch (...) {
            isInRealFunction = false;
        }

        if (isInRealFunction)
        {
            if (ctx.variableTracker.existsInCurrentScope(name))
            {
                throw errors::EnvironmentException(
                    "Variable '" + name + "' is already defined in this scope",
                    node->getLocation()
                );
            }

            // C# semantics: lambdas cannot shadow outer-scope captured variables.
            if (ctx.functionFrameManager.isInLambda())
            {
                const auto& frame = ctx.functionFrameManager.currentFrame();
                for (const auto& capturedName : frame.capturedVariableNames)
                {
                    if (name == capturedName)
                    {
                        throw errors::TypeException(
                            "Local variable '" + name + "' shadows outer scope variable. "
                            "Variable shadowing is not allowed in lambdas (C# semantics).",
                            node->getLocation());
                    }
                }
            }

            ctx.variableTracker.declareLocal(name, varType, node->getClassName(), node->isNullableType(), node->getIsFinal());
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

            size_t absoluteSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            size_t relativeSlot = absoluteSlot - ctx.functionFrameManager.currentFrame().localStartSlot;
            size_t nameIndex = ctx.program.getConstantPool().addString(name);
            const auto& frame = ctx.functionFrameManager.currentFrame();
            if (frame.scopeDepthStart == 0 && frame.localStartSlot == 0)
            {
                ctx.program.setTopLevelLocalName(relativeSlot, name);
            }
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL,
                                         static_cast<uint64_t>(relativeSlot),
                                         static_cast<uint64_t>(nameIndex), node);
            return;
        }

        if (ctx.globalRegistry.existsInCurrentScope(name, ctx.variableTracker.getCurrentScopeDepth()))
        {
            throw errors::EnvironmentException(
                "Variable '" + name + "' is already defined in this scope",
                node->getLocation()
            );
        }

        int scopeDepth = ctx.variableTracker.getCurrentScopeDepth();
        ctx.globalRegistry.registerGlobal(name, varType, node->getClassName(), scopeDepth, node->isNullableType());

        std::string typeName = node->getClassName().empty() ? "auto" : node->getClassName();
        vm::bytecode::BytecodeProgram::GlobalVariableMetadata globalMeta;
        globalMeta.name = name;
        globalMeta.typeName = typeName;
        globalMeta.type = varType;
        globalMeta.isFinal = node->getIsFinal();
        ctx.program.registerGlobalVariable(globalMeta);

        size_t nameIndex = ctx.program.getConstantPool().addString(name);
        size_t typeIndex = ctx.program.getConstantPool().addString("auto");
        uint32_t isFinal = node->getIsFinal() ? 1 : 0;

        ctx.emitter.emitWithLocation(bytecode::OpCode::DECLARE_VAR,
                                     std::vector<uint64_t>{
                                         static_cast<uint64_t>(nameIndex),
                                         static_cast<uint64_t>(typeIndex),
                                         isFinal
                                     }, node);
    }

    void StatementCompiler::emitVariableReassignment(ast::AssignmentNode* node, bool isReassignment)
    {
        std::string name = node->getVariableName();
        value::ValueType varType = node->getVariableType();

        // Static field assignment: ClassName::fieldName.
        if (name.find("::") != std::string::npos)
        {
            size_t nameIndex = ctx.program.getConstantPool().addString(name);
            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_STATIC, static_cast<uint64_t>(nameIndex), node);
            return;
        }

        size_t localSlot = SIZE_MAX;
        if (ctx.functionFrameManager.isInFunction())
        {
            localSlot = ctx.variableTracker.resolveLocal(name,
                                                         ctx.functionFrameManager.currentFrame().localStartSlot);
        }

        if (localSlot != SIZE_MAX)
        {
            // MYT-247: reject reassignment of a final local at compile time.
            size_t startSlot = ctx.functionFrameManager.currentFrame().localStartSlot;
            if (ctx.variableTracker.isLocalFinal(name, startSlot))
            {
                throw errors::TypeException(
                    "Cannot reassign final local variable '" + name + "'",
                    node->getLocation()
                );
            }

            // MYT-215: mark mutated so the lambda-capture-in-loop check can
            // reject the captures.
            size_t absoluteSlot = localSlot + ctx.functionFrameManager.currentFrame().localStartSlot;
            ctx.variableTracker.markVariableAsMutated(absoluteSlot);

            size_t nameIndex = ctx.program.getConstantPool().addString(name);
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL,
                                         static_cast<uint64_t>(localSlot),
                                         static_cast<uint64_t>(nameIndex), node);
            return;
        }

        // Check instance fields BEFORE treating as type-inferred local — fields
        // would otherwise be incorrectly registered as local variables. The
        // runtime classRegistry isn't populated yet at compile time, so walk
        // the current ClassNode's AST directly.
        if (ctx.currentClassNode && ctx.functionFrameManager.isInFunction())
        {
            const auto& fields = ctx.currentClassNode->getFields();
            for (const auto& fieldPtr : fields)
            {
                if (auto* fieldNode = dynamic_cast<ast::FieldNode*>(fieldPtr.get()))
                {
                    if (fieldNode->getName() == name)
                    {
                        if (fieldNode->getIsFinal())
                        {
                            throw errors::TypeException(
                                "Cannot modify final field '" + name + "' of class '" +
                                ctx.currentClassNode->getClassName() + "'",
                                node->getLocation()
                            );
                        }

                        if (fieldNode->getIsStatic())
                        {
                            std::string qualifiedName = ctx.currentClassNode->getClassName() + "::" + name;
                            size_t nameIndex = ctx.program.getConstantPool().addString(qualifiedName);
                            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_STATIC, static_cast<uint64_t>(nameIndex),
                                                         node);
                            return;
                        }
                        else
                        {
                            // Instance field: LOAD_LOCAL 0 (this), value, SET_FIELD
                            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
                            ctx.emitter.emitWithLocation(bytecode::OpCode::SWAP, node);
                            size_t fieldNameIndex = ctx.program.getConstantPool().addString(name);
                            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD,
                                                         static_cast<uint64_t>(fieldNameIndex), node);
                            return;
                        }
                    }
                }
            }

            if (ctx.currentClassNode->hasParentClass())
            {
                auto classRegistry = ctx.env->getClassRegistry();
                if (classRegistry)
                {
                    std::string parentClassName = ctx.currentClassNode->getParentClassName();
                    auto parentClass = classRegistry->findClass(parentClassName);

                    while (parentClass)
                    {
                        auto field = parentClass->getField(name);
                        if (field)
                        {
                            if (field->isFinal())
                            {
                                throw errors::TypeException(
                                    "Cannot modify final field '" + name + "' inherited from class '" +
                                    parentClass->getName() + "'",
                                    node->getLocation()
                                );
                            }

                            if (field->isStatic())
                            {
                                std::string qualifiedName = parentClass->getName() + "::" + name;
                                size_t nameIndex = ctx.program.getConstantPool().addString(qualifiedName);
                                ctx.emitter.emitWithLocation(bytecode::OpCode::SET_STATIC,
                                                             static_cast<uint64_t>(nameIndex), node);
                                return;
                            }
                            else
                            {
                                ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
                                ctx.emitter.emitWithLocation(bytecode::OpCode::SWAP, node);
                                size_t fieldNameIndex = ctx.program.getConstantPool().addString(name);
                                ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD,
                                                             static_cast<uint64_t>(fieldNameIndex), node);
                                return;
                            }
                        }

                        parentClass = parentClass->hasParentClass() ? parentClass->getParentClass() : nullptr;
                    }
                }
            }
        }

        // In a real function with no existing var and not a reassignment: new
        // type-inferred local declaration. Global scope has an empty function name.
        std::string currentFunctionName = ctx.functionFrameManager.getCurrentFunctionName();
        bool isRealFunction = ctx.functionFrameManager.isInFunction() && !currentFunctionName.empty();
        if (isRealFunction && !isReassignment)
        {
            if (ctx.variableTracker.existsInCurrentScope(name))
            {
                throw errors::EnvironmentException(
                    "Variable '" + name + "' is already defined in this scope",
                    node->getLocation()
                );
            }

            if (ctx.functionFrameManager.isInLambda())
            {
                const auto& frame = ctx.functionFrameManager.currentFrame();
                for (const auto& capturedName : frame.capturedVariableNames)
                {
                    if (name == capturedName)
                    {
                        throw errors::TypeException(
                            "Local variable '" + name + "' shadows outer scope variable. "
                            "Variable shadowing is not allowed in lambdas (C# semantics).",
                            node->getLocation());
                    }
                }
            }

            ctx.variableTracker.declareLocal(name, varType, node->getClassName(), node->isNullableType(), node->getIsFinal());
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

            size_t absoluteSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            size_t relativeSlot = absoluteSlot - ctx.functionFrameManager.currentFrame().localStartSlot;
            size_t nameIndex = ctx.program.getConstantPool().addString(name);
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL,
                                         static_cast<uint64_t>(relativeSlot),
                                         static_cast<uint64_t>(nameIndex), node);
            return;
        }

        if (!ctx.globalRegistry.exists(name))
        {
            throw errors::UndefinedException(
                "Cannot assign to undeclared variable '" + name + "'. "
                "Did you forget to declare it with a type?",
                node->getLocation()
            );
        }

        size_t nameIndex = ctx.program.getConstantPool().addString(name);
        ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_VAR, static_cast<uint64_t>(nameIndex), node);
    }
}
