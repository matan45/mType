#include "ControlFlowCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "StatementCleanup.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../../errors/TypeException.hpp"

namespace vm::compiler::visitors
{
    value::Value ControlFlowCompiler::compileForEach(ast::ForEachNode* node)
    {
        std::string varName = node->getVariableName();
        value::ValueType varType = node->getVariableType();

        ctx.variableTracker.beginScope();

        // Evaluate collection
        node->getCollection()->accept(ctx.visitor);

        // Determine if we're iterating over an array or a collection.
        // Array indicators: explicit ARRAY type, IndexAccessNode collection,
        // toArray() method call, or "[]" in the inferred type name.

        const auto& varTypeInfo = node->getVariableTypeInfo();
        bool isArrayFromTypeInfo = (varType == value::ValueType::ARRAY);

        bool isArrayType = (isArrayFromTypeInfo ||
                           dynamic_cast<ast::IndexAccessNode*>(node->getCollection()) != nullptr);

        if (auto methodCall = dynamic_cast<ast::MethodCallNode*>(node->getCollection()))
        {
            if (methodCall->getMethodName() == "toArray")
            {
                isArrayType = true;
            }
        }

        // Check if the collection is a variable and look up its type
        if (auto varNode = dynamic_cast<ast::nodes::expressions::VariableNode*>(node->getCollection()))
        {
            std::string varName = varNode->getName();

            const auto& locals = ctx.variableTracker.getLocals();
            for (const auto& local : locals)
            {
                if (local.name == varName)
                {
                    if (local.type == value::ValueType::ARRAY)
                    {
                        isArrayType = true;
                    }
                    break;
                }
            }
        }

        // Also check if type inference reports this as an array type (e.g., "T[]", "Int[]", "String[]")
        if (!isArrayType) {
            std::string collectionClassName = ctx.typeInference.inferExpressionClassName(node->getCollection());
            if (collectionClassName.find("[]") != std::string::npos) {
                isArrayType = true;
            }
        }

        std::string collectionClassNameForFastPath =
            ctx.typeInference.inferExpressionClassName(node->getCollection());
        std::string collectionBaseClassName = collectionClassNameForFastPath;
        size_t collectionGenericStart = collectionBaseClassName.find('<');
        if (collectionGenericStart != std::string::npos) {
            collectionBaseClassName = collectionBaseClassName.substr(0, collectionGenericStart);
        }
        bool isArrayListType = !isArrayType && collectionBaseClassName == "ArrayList";

        if (!isArrayType) {
            std::string collectionClassName = collectionClassNameForFastPath;
            if (collectionClassName.empty()) {
                collectionClassName = "Object";
            }

            std::string loopVarTypeName;
            if (varTypeInfo.baseType == value::ValueType::OBJECT && !varTypeInfo.className.empty()) {
                loopVarTypeName = varTypeInfo.className;
            } else {
                loopVarTypeName = varTypeInfo.toString();
            }

            try {
                ctx.typeValidator.validateAndExtractIterableElementType(
                    collectionClassName,
                    loopVarTypeName,
                    node->getLocation()
                );
            } catch (const errors::TypeException& e) {
                throw errors::TypeException(e.what(), node->getLocation());
            }
        }

        if (isArrayType) {
            // === ARRAY FAST PATH ===
            // Counter-based iteration with ARRAY_GET.

            // Store collection in local. STORE_LOCAL re-pushes the stored value
            // (see VariableExecutor::handleStoreLocal), so each setup store
            // here needs a POP to keep the operand stack clean — without it,
            // foreach loops leak setup values across every iteration and OSR
            // can't tier them up.
            ctx.variableTracker.declareLocal("__foreach_array__", value::ValueType::OBJECT, "");
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t arraySlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(arraySlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

            // Get array length
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(arraySlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::ARRAY_LENGTH, node);

            // Store length in local
            ctx.variableTracker.declareLocal("__foreach_length__", value::ValueType::INT, "");
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t lengthSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(lengthSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

            // Initialize counter
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT,
                           static_cast<uint64_t>(ctx.program.getConstantPool().addInteger(0)), node);
            ctx.variableTracker.declareLocal("__foreach_counter__", value::ValueType::INT, "");
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t counterSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(counterSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

            ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_START, node);

            // Loop start
            size_t loopStart = ctx.program.getCurrentOffset();
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(counterSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(lengthSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::LT, node);

            size_t exitJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

            // Get current element
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(arraySlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(counterSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::ARRAY_GET, node);

            // Store in loop variable (per-iter — needs POP for STORE_LOCAL re-push)
            ctx.variableTracker.declareLocal(varName, varType, "");
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t loopVarSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(loopVarSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

            // Enter loop context
            size_t continueTarget = ctx.program.getCurrentOffset();
            ctx.loopManager.enterLoop(loopStart, continueTarget);

            // Compile body
            // MYT-271: braceless foreach-bodies need the same cleanup as braced.
            if (node->getBody()) {
                size_t feBodyOffsetBefore = ctx.program.getCurrentOffset();
                node->getBody()->accept(ctx.visitor);
                statementCleanup::emitStatementCleanup(ctx, node->getBody(), feBodyOffsetBefore);
            }

            // `continue` inside the body must jump to the counter increment, not to the
            // top of the body — otherwise the counter never advances and we infinite-loop.
            size_t incrementStart = ctx.program.getCurrentOffset();
            for (size_t continueJump : ctx.loopManager.getContinueJumps()) {
                ctx.program.patchJump(continueJump, static_cast<uint64_t>(incrementStart));
            }

            // Increment counter
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(counterSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT,
                           static_cast<uint64_t>(ctx.program.getConstantPool().addInteger(1)), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::ADD, node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(counterSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

            // Jump back to loop start
            ctx.emitter.emitLoop(loopStart);

            ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_END, node);

            // Patch exit jump (target for the loop-condition exit)
            ctx.emitter.patchJump(exitJump);

            // `break` jumps land here — past the loop entirely.
            for (size_t breakJump : ctx.loopManager.getBreakJumps()) {
                ctx.emitter.patchJump(breakJump);
            }

            ctx.loopManager.exitLoop();
        }
        else if (isArrayListType) {
            // === ARRAYLIST FAST PATH ===
            // ArrayList.iterator() snapshots (data, count) into ListIterator.
            // Lower the foreach to that same shape directly: one count/data
            // snapshot followed by indexed array reads.
            ctx.variableTracker.declareLocal("__foreach_arraylist__", value::ValueType::OBJECT, collectionClassNameForFastPath);
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t listSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(listSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

            size_t dataFieldIndex = ctx.program.getConstantPool().addString("data");
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(listSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::INLINE_GET_FIELD, static_cast<uint64_t>(dataFieldIndex), node);

            ctx.variableTracker.declareLocal("__foreach_arraylist_data__", value::ValueType::ARRAY, "");
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t dataSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(dataSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

            size_t countFieldIndex = ctx.program.getConstantPool().addString("count");
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(listSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::INLINE_GET_FIELD, static_cast<uint64_t>(countFieldIndex), node);

            ctx.variableTracker.declareLocal("__foreach_length__", value::ValueType::INT, "");
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t lengthSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(lengthSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT,
                           static_cast<uint64_t>(ctx.program.getConstantPool().addInteger(0)), node);
            ctx.variableTracker.declareLocal("__foreach_counter__", value::ValueType::INT, "");
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t counterSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(counterSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

            ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_START, node);

            size_t loopStart = ctx.program.getCurrentOffset();
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(counterSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(lengthSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::LT, node);

            size_t exitJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(dataSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(counterSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::ARRAY_GET, node);

            ctx.variableTracker.declareLocal(varName, varType, "");
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t loopVarSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(loopVarSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

            size_t continueTarget = ctx.program.getCurrentOffset();
            ctx.loopManager.enterLoop(loopStart, continueTarget);

            if (node->getBody()) {
                size_t feBodyOffsetBefore = ctx.program.getCurrentOffset();
                node->getBody()->accept(ctx.visitor);
                statementCleanup::emitStatementCleanup(ctx, node->getBody(), feBodyOffsetBefore);
            }

            size_t incrementStart = ctx.program.getCurrentOffset();
            for (size_t continueJump : ctx.loopManager.getContinueJumps()) {
                ctx.program.patchJump(continueJump, static_cast<uint64_t>(incrementStart));
            }

            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(counterSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT,
                           static_cast<uint64_t>(ctx.program.getConstantPool().addInteger(1)), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::ADD, node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(counterSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

            ctx.emitter.emitLoop(loopStart);

            ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_END, node);

            ctx.emitter.patchJump(exitJump);

            for (size_t breakJump : ctx.loopManager.getBreakJumps()) {
                ctx.emitter.patchJump(breakJump);
            }

            ctx.loopManager.exitLoop();
        }
        else {
            // === ITERATOR PATH ===
            // Use GET_ITERATOR, ITERATOR_HAS_NEXT, ITERATOR_NEXT opcodes.
            // Collection is already on the stack from node->getCollection()->accept().
            ctx.emitter.emitWithLocation(bytecode::OpCode::GET_ITERATOR, node);

            // Determine if collection is a known non-nullable variable
            bool iteratorNonNull = false;
            if (auto* collectionVar = dynamic_cast<ast::VariableNode*>(node->getCollection()))
            {
                const std::string& collVarName = collectionVar->getName();
                if (ctx.nullNarrowing.isNarrowedNonNull(collVarName))
                {
                    iteratorNonNull = true;
                }
                else if (ctx.variableTracker.existsInFunction(collVarName))
                {
                    iteratorNonNull = !ctx.variableTracker.getLocalNullableByName(collVarName);
                }
                else if (ctx.globalRegistry.exists(collVarName))
                {
                    iteratorNonNull = !ctx.globalRegistry.isNullable(collVarName);
                }
                if (iteratorNonNull)
                {
                    ctx.program.setLastInstructionFlags(bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER);
                }
            }

            // Store iterator in local variable (STORE_LOCAL re-pushes — POP it)
            ctx.variableTracker.declareLocal("__foreach_iterator__", value::ValueType::OBJECT, "");
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t iteratorSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(iteratorSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

            ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_START, node);

            // Loop start - check hasNext()
            size_t loopStart = ctx.program.getCurrentOffset();
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(iteratorSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::ITERATOR_HAS_NEXT, node);
            if (iteratorNonNull)
            {
                ctx.program.setLastInstructionFlags(bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER);
            }

            // Jump if false (no more elements)
            size_t exitJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

            // Get next element
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(iteratorSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::ITERATOR_NEXT, node);
            if (iteratorNonNull)
            {
                ctx.program.setLastInstructionFlags(bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER);
            }

            // Store in loop variable (per-iter — needs POP for STORE_LOCAL re-push)
            ctx.variableTracker.declareLocal(varName, varType, "");
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t loopVarSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(loopVarSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

            // Enter loop context
            size_t continueTarget = ctx.program.getCurrentOffset();
            ctx.loopManager.enterLoop(loopStart, continueTarget);

            // Compile body
            // MYT-271: braceless foreach-bodies need the same cleanup as braced.
            if (node->getBody()) {
                size_t feBodyOffsetBefore = ctx.program.getCurrentOffset();
                node->getBody()->accept(ctx.visitor);
                statementCleanup::emitStatementCleanup(ctx, node->getBody(), feBodyOffsetBefore);
            }

            // `continue` inside the body re-runs the hasNext check at loopStart.
            for (size_t continueJump : ctx.loopManager.getContinueJumps()) {
                ctx.program.patchJump(continueJump, static_cast<uint64_t>(loopStart));
            }

            // Jump back to loop start (hasNext check)
            ctx.emitter.emitLoop(loopStart);

            ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_END, node);

            // Patch exit jump
            ctx.emitter.patchJump(exitJump);

            // `break` lands here so it still falls through to ITERATOR_CLOSE — otherwise
            // the iterator's resources leak when a loop is exited via break.
            for (size_t breakJump : ctx.loopManager.getBreakJumps()) {
                ctx.emitter.patchJump(breakJump);
            }

            // Cleanup: Close the iterator
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(iteratorSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::ITERATOR_CLOSE, node);

            ctx.loopManager.exitLoop();
        }

        ctx.variableTracker.endScope();

        return std::monostate{};
    }
}
