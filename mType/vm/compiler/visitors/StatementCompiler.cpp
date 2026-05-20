#include "StatementCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "StatementCleanup.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/UndefinedException.hpp"
#include "../validation/CompileTimeValidator.hpp"

namespace vm::compiler::visitors
{
    StatementCompiler::StatementCompiler(CompilerContext& context)
        : ctx(context)
    {
    }

    bool StatementCompiler::detectReassignment(const std::string& name, std::string& existingClassName)
    {
        bool isReassignment = false;

        if (ctx.functionFrameManager.isInFunction())
        {
            existingClassName = ctx.variableTracker.getLocalClassNameByName(name);
            if (!existingClassName.empty() || ctx.variableTracker.existsInCurrentScope(name))
            {
                isReassignment = true;
            }
        }

        if (!isReassignment && ctx.globalRegistry.exists(name))
        {
            isReassignment = true;
            existingClassName = ctx.globalRegistry.getClassName(name);
        }

        return isReassignment;
    }

    void StatementCompiler::validateClassOrInterfaceType(ast::AssignmentNode* node)
    {
        value::ValueType varType = node->getVariableType();

        if (varType == value::ValueType::OBJECT && !node->getClassName().empty())
        {
            std::string className = node->getClassName();
            bool isArrayType = className.find("[]") != std::string::npos;
            bool isGenericParam = (className.length() == 1 && std::isupper(className[0]));

            if (!isArrayType && !isGenericParam)
            {
                std::string baseClassName = ctx.typeSubstitutionService.extractBaseTypeName(className);

                if (ctx.compileTimeValidator)
                {
                    ctx.compileTimeValidator->validateTypeIsNotRawGeneric(className, node->getLocation());
                }

                // Promise is native async/await support — not a user-declarable class.
                if (baseClassName != "Promise")
                {
                    if (!ctx.env->findClass(baseClassName))
                    {
                        const auto& classes = ctx.program.getClasses();
                        bool found = false;
                        for (const auto& classMeta : classes)
                        {
                            if (classMeta.name == baseClassName)
                            {
                                found = true;
                                break;
                            }
                        }
                        if (!found && !ctx.env->findInterface(baseClassName))
                        {
                            throw errors::UndefinedException(
                                "Undefined class or interface: '" + baseClassName + "'",
                                node->getLocation()
                            );
                        }
                    }
                }
            }
        }
    }

    value::Value StatementCompiler::compileBlock(ast::BlockNode* node)
    {
        // Function body blocks must NOT create their own scope — the function
        // already created one, and a second beginScope here would clear all
        // variables before FunctionCompiler can capture their names. Anonymous
        // blocks SHOULD create their own scopes, even inside functions.
        bool shouldManageScope = false;

        if (!ctx.functionFrameManager.isInFunction())
        {
            shouldManageScope = true;
        }
        else
        {
            const auto& frame = ctx.functionFrameManager.currentFrame();
            // Global pseudo-frame (scopeDepthStart=0, localStartSlot=0): anonymous
            // blocks manage their own scopes.
            if (frame.scopeDepthStart == 0 && frame.localStartSlot == 0)
            {
                shouldManageScope = true;
            }
        }

        // Per-scope NEW_STACK release: emit ENTER before this block's
        // statements and LEAVE after, whenever the block actually contains a
        // promoted NEW (EscapeAnalysisPass marks this). The variable-tracker
        // beginScope/endScope is independently gated on shouldManageScope;
        // the two concerns are decoupled. MYT-352: function-body blocks must
        // also wrap, otherwise an inlined callee leaks pool slots into the
        // caller's frame on every iteration (frame teardown would otherwise
        // be the implicit backstop, but inlining elides it).
        //
        // MYT-352 follow-up: also gate on the runtime cap
        // (kCompilerStackObjectScopeStackCap, mirroring
        // CallFrame::kStackObjectScopeStackCap). At depth >= cap the runtime
        // ENTER is silently skipped while the matching LEAVE would still
        // pop, desynchronising the scope stack and double-releasing pool
        // slots. Opting out at compile time falls back to frame-teardown
        // cleanup (the cap-then-heap fallback path documented in
        // ExecutionContext.hpp) — correct, just no per-scope reclamation
        // for the over-deep nest.
        const bool emitStackScope =
            kEmitStackScopeOps && node->containsStackAlloc() &&
            ctx.loopManager.getCurrentStackScopeDepth() <
                kCompilerStackObjectScopeStackCap;

        if (shouldManageScope)
        {
            ctx.variableTracker.beginScope();
        }

        if (emitStackScope)
        {
            ctx.emitter.emitWithLocation(bytecode::OpCode::STACK_SCOPE_ENTER, node);
            ctx.loopManager.notifyStackScopeEnter();
        }

        const auto& statements = node->getStatements();
        for (auto& stmt : statements)
        {
            size_t offsetBefore = ctx.program.getCurrentOffset();
            stmt->accept(ctx.visitor);
            emitStatementCleanup(stmt.get(), offsetBefore);
        }

        if (emitStackScope)
        {
            ctx.emitter.emitWithLocation(bytecode::OpCode::STACK_SCOPE_LEAVE, node);
            ctx.loopManager.notifyStackScopeLeave();
        }

        if (shouldManageScope)
        {
            ctx.variableTracker.endScope();
            ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());
        }

        return std::monostate{};
    }

    value::Value StatementCompiler::compileProgram(ast::ProgramNode* node)
    {
        // Add a scope only when compiling the MAIN file (at scope 2). Imported files
        // are already at scope 3, so they shouldn't add another scope.
        bool shouldManageScope = (ctx.variableTracker.getCurrentScopeDepth() == 2);

        if (shouldManageScope) {
            ctx.variableTracker.beginScope();
        }

        const auto& statements = node->getStatements();
        for (auto& stmt : statements)
        {
            size_t offsetBefore = ctx.program.getCurrentOffset();
            stmt->accept(ctx.visitor);
            emitStatementCleanup(stmt.get(), offsetBefore);
        }

        if (shouldManageScope) {
            ctx.variableTracker.endScope();
        }

        ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

        return std::monostate{};
    }

    bool StatementCompiler::tryEmitAutoBoxing(ast::ASTNode* valueNode, const std::string& targetClassName)
    {
        bool isBoxType = (targetClassName == "Int" ||
                          targetClassName == "Float" ||
                          targetClassName == "Bool" ||
                          targetClassName == "String");

        if (!isBoxType || !valueNode)
        {
            return false;
        }

        value::ValueType valueType = ctx.typeInference.inferExpressionType(valueNode);
        bool needsBoxing = false;

        if (targetClassName == "Int" && valueType == value::ValueType::INT) needsBoxing = true;
        else if (targetClassName == "Float" && valueType == value::ValueType::FLOAT) needsBoxing = true;
        else if (targetClassName == "Bool" && valueType == value::ValueType::BOOL) needsBoxing = true;
        else if (targetClassName == "String" && valueType == value::ValueType::STRING) needsBoxing = true;

        if (!needsBoxing)
        {
            return false;
        }

        valueNode->accept(ctx.visitor);

        size_t classNameIndex = ctx.program.getConstantPool().addString(targetClassName);
        auto boxClassDef = ctx.env->findClass(targetClassName);
        bool boxIsValue = boxClassDef && boxClassDef->isValueClass();
        if (boxIsValue) {
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_VALUE_OBJECT,
                                         static_cast<uint64_t>(classNameIndex),
                                         1u, valueNode);
            ctx.emitter.emitWithLocation(bytecode::OpCode::OBJECT_TO_VALUE, valueNode);
        } else {
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                         static_cast<uint64_t>(classNameIndex),
                                         1u, valueNode);
        }

        return true;
    }

    void StatementCompiler::emitStatementCleanup(ast::ASTNode* stmt, size_t offsetBefore)
    {
        statementCleanup::emitStatementCleanup(ctx, stmt, offsetBefore);
    }

    bool StatementCompiler::isExpressionStatement(ast::ASTNode* node) const
    {
        return statementCleanup::isExpressionStatement(node);
    }
}
