#include "TryStatementHelper.hpp"
#include <cstddef>
#include <cstdint>
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/TypeException.hpp"

namespace vm::compiler::visitors
{
    TryStatementHelper::TryStatementHelper(CompilerContext& context)
        : ctx(context)
    {
    }

    value::Value TryStatementHelper::compileTry(ast::TryNode* node)
    {
        // Emit TRY_BEGIN instruction
        size_t tryBeginOffset = ctx.program.getCurrentOffset();
        ctx.emitter.emitWithLocation(bytecode::OpCode::TRY_BEGIN, node);

        // Enter exception context (tell it if there's a finally block)
        bool hasFinally = (node->getFinallyBlock() != nullptr);
        ctx.exceptionManager.enterTry(tryBeginOffset, hasFinally);

        // Get nesting level for exception table
        uint32_t nestingLevel = ctx.exceptionManager.getNestingLevel();

        // Enter scope for try block (variables shouldn't leak to catch/finally)
        ctx.variableTracker.beginScope();

        // Compile try block
        node->getTryBlock()->accept(ctx.visitor);

        // Exit try block scope
        ctx.variableTracker.endScope();
        ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

        // Mark end of try block
        size_t tryEndOffset = ctx.program.getCurrentOffset();
        ctx.exceptionManager.setTryEndOffset(tryEndOffset);

        // Jump over catch blocks if no exception thrown
        size_t endJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
        ctx.exceptionManager.registerExitJump(endJump);

        // Emit TRY_END instruction
        ctx.emitter.emitWithLocation(bytecode::OpCode::TRY_END, node);

        // Compile catch blocks and build exception table entries, collecting their IP ranges
        bool noCatchBlocks = node->getCatchBlocks().empty();
        std::vector<CatchBlockRange> catchRanges;
        if (!noCatchBlocks) {
            catchRanges = compileCatchBlocks(node->getCatchBlocks(), tryBeginOffset, tryEndOffset, nestingLevel);
        }

        // Compile finally block if present, passing catch ranges so finally can protect them too
        compileFinallyBlock(dynamic_cast<ast::BlockNode*>(node->getFinallyBlock()), tryBeginOffset, tryEndOffset, nestingLevel, noCatchBlocks, catchRanges);

        // Exit exception context
        ctx.exceptionManager.exitTry();

        return std::monostate{};
    }

    std::vector<TryStatementHelper::CatchBlockRange> TryStatementHelper::compileCatchBlocks(const std::vector<std::unique_ptr<ast::CatchNode>>& catchBlocks,
                                                size_t tryBeginOffset, size_t tryEndOffset, uint32_t nestingLevel)
    {
        std::vector<CatchBlockRange> catchRanges;

        for (const auto& catchBlock : catchBlocks) {
            size_t catchHandlerOffset = ctx.program.getCurrentOffset();

            // Validate that the catch type is Exception or inherits from Exception
            std::string exceptionType = catchBlock->getExceptionType();

            // Extract base class name from generic type (e.g., "ResultException<Int>" -> "ResultException")
            std::string baseClassName = exceptionType;
            size_t genericStart = exceptionType.find('<');
            if (genericStart != std::string::npos)
            {
                baseClassName = exceptionType.substr(0, genericStart);
            }

            auto classRegistry = ctx.env->getClassRegistry();
            auto exceptionClass = classRegistry->findClass(baseClassName);

            if (!exceptionClass)
            {
                throw errors::TypeException(
                    "Cannot catch undefined type '" + exceptionType + "'. "
                    "Catch type must be a class that exists in the environment.",
                    catchBlock->getLocation()
                );
            }

            // Check if the class inherits from Exception
            bool inheritsFromException = false;
            auto currentClass = exceptionClass;
            while (currentClass)
            {
                if (currentClass->getName() == "Exception")
                {
                    inheritsFromException = true;
                    break;
                }

                // Check parent class
                if (currentClass->hasParentClass())
                {
                    currentClass = classRegistry->findClass(currentClass->getParentClassName());
                }
                else
                {
                    break;
                }
            }

            if (!inheritsFromException)
            {
                throw errors::TypeException(
                    "Cannot catch type '" + exceptionType + "'. "
                    "Catch blocks can only catch Exception or classes that inherit from Exception.",
                    catchBlock->getLocation()
                );
            }

            // MYT-245: resolve aliased catch type to canonical class name. The throw
            // site (NEW_OBJECT) emits canonical names, and ExceptionTable::isTypeCompatible
            // string-compares the catch entry's exceptionType against the thrown type. If
            // this catch was written with an aliased class (e.g. `catch (Boom e)` where
            // Boom is `import {CustomException as Boom}`), findClass returned the canonical
            // class def via the alias registration done by applyImportAliases — use its
            // canonical name everywhere downstream.
            const std::string canonicalBase = exceptionClass->getName();
            if (canonicalBase != baseClassName)
            {
                if (genericStart != std::string::npos)
                {
                    exceptionType = canonicalBase + exceptionType.substr(genericStart);
                }
                else
                {
                    exceptionType = canonicalBase;
                }
            }

            // Register catch handler
            ctx.exceptionManager.registerCatchHandler(
                exceptionType,
                catchBlock->getVariableName(),
                catchHandlerOffset
            );

            // Emit CATCH instruction with exception type
            uint64_t typeIndex = static_cast<uint64_t>(ctx.program.getConstantPool().addString(exceptionType));
            ctx.emitter.emitWithLocation(bytecode::OpCode::CATCH, typeIndex, catchBlock.get());

            // Enter scope for catch variable
            ctx.variableTracker.beginScope();

            // Declare catch variable (exception is on stack)
            // Note: Only declare in variableTracker, NOT in globalRegistry
            // This ensures it's treated as a local variable (LOAD_LOCAL) not global (LOAD_VAR)
            ctx.variableTracker.declareLocal(
                catchBlock->getVariableName(),
                value::ValueType::OBJECT,
                catchBlock->getExceptionType()
            );

            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t catchVarSlot = ctx.variableTracker.getNextLocalSlot() - 1;

            // Convert absolute slot to relative slot for STORE_LOCAL emission
            size_t relativeSlot = catchVarSlot;
            if (ctx.functionFrameManager.isInFunction()) {
                size_t startSlot = ctx.functionFrameManager.currentFrame().localStartSlot;
                relativeSlot = catchVarSlot - startSlot;
            }

            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(relativeSlot), catchBlock.get());

            // Build exception table entry for this catch block
            bytecode::ExceptionTableEntry entry(
                tryBeginOffset,                  // startIP
                tryEndOffset,                    // endIP
                catchHandlerOffset,              // catchIP
                SIZE_MAX,                        // finallyIP (will be updated later if there's a finally block)
                exceptionType,                   // exceptionType
                nestingLevel,                    // nestingLevel
                catchBlock->getVariableName(),   // catchVarName
                relativeSlot                     // catchVarSlot (use relative slot)
            );

            // Add entry to appropriate exception table (function or global)
            std::string currentFunctionName = ctx.functionFrameManager.getCurrentFunctionName();
            if (currentFunctionName.empty()) {
                // Global scope
                ctx.program.getGlobalExceptionTable().addEntry(entry);
            } else {
                // Function scope - get function metadata and add to its exception table
                // Note: Function is pre-registered during FunctionRegistrar phase, so it exists
                auto* funcMetadata = const_cast<bytecode::BytecodeProgram::FunctionMetadata*>(
                    ctx.program.getFunction(currentFunctionName)
                );
                if (funcMetadata) {
                    funcMetadata->exceptionTable.addEntry(entry);
                }
            }

            // Track start of catch body for finally exception table entries
            size_t catchBodyStart = ctx.program.getCurrentOffset();

            // Compile catch body
            catchBlock->getBody()->accept(ctx.visitor);

            // Track end of catch body
            size_t catchBodyEnd = ctx.program.getCurrentOffset();
            catchRanges.push_back({catchBodyStart, catchBodyEnd});

            // Exit catch scope
            ctx.variableTracker.endScope();
            ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

            // Jump to end after catch block
            size_t catchEndJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
            ctx.exceptionManager.registerExitJump(catchEndJump);
        }

        return catchRanges;
    }
}
