#include "ControlFlowCompiler.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/ParseException.hpp"
#include "../../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../token/TokenType.hpp"

namespace vm::compiler::visitors
{
    ControlFlowCompiler::ControlFlowCompiler(CompilerContext& context)
        : ctx(context)
        , tryHelper(std::make_unique<TryStatementHelper>(context))
    {
    }

    value::Value ControlFlowCompiler::compileIf(ast::IfNode* node)
    {
        // Compile condition
        node->getCondition()->accept(ctx.visitor);  // Will need delegation

        // PHASE 4: Auto-unbox Bool objects to primitive bool
        value::ValueType conditionType = ctx.typeInference.inferExpressionType(node->getCondition());
        if (conditionType == value::ValueType::OBJECT)
        {
            std::string conditionClassName = ctx.typeInference.inferExpressionClassName(node->getCondition());
            if (conditionClassName == "Bool")
            {
                // Auto-unbox: call getValue() to get primitive bool
                size_t methodNameIndex = ctx.program.getConstantPool().addString("getValue");
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint64_t>(methodNameIndex),
                                             0u,  // 0 arguments
                                             node->getCondition());
            }
        }

        // Jump to else/end if condition is false
        size_t elseJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        // Analyze condition for null narrowing (smart casts)
        // Detect patterns: x != null (narrow in then-branch) or x == null (narrow in else-branch)
        std::string narrowVarName;
        bool narrowInThen = false;
        bool narrowInElse = false;
        if (auto* binExpr = dynamic_cast<ast::nodes::expressions::BinaryExpNode*>(node->getCondition()))
        {
            auto* left = binExpr->getLeft();
            auto* right = binExpr->getRight();
            token::TokenType op = binExpr->getOperator();

            bool leftIsNull = dynamic_cast<ast::NullNode*>(left) != nullptr;
            bool rightIsNull = dynamic_cast<ast::NullNode*>(right) != nullptr;
            auto* leftVar = dynamic_cast<ast::nodes::expressions::VariableNode*>(left);
            auto* rightVar = dynamic_cast<ast::nodes::expressions::VariableNode*>(right);

            if (op == token::TokenType::NOT_EQUALS)
            {
                if (rightIsNull && leftVar) { narrowVarName = leftVar->getName(); narrowInThen = true; }
                else if (leftIsNull && rightVar) { narrowVarName = rightVar->getName(); narrowInThen = true; }
            }
            else if (op == token::TokenType::EQUALS)
            {
                if (rightIsNull && leftVar) { narrowVarName = leftVar->getName(); narrowInElse = true; }
                else if (leftIsNull && rightVar) { narrowVarName = rightVar->getName(); narrowInElse = true; }
            }
        }

        // Compile then branch with its own scope
        // This ensures variables declared in the if block don't leak out
        ctx.variableTracker.beginScope();
        if (narrowInThen && !narrowVarName.empty())
        {
            ctx.nullNarrowing.enterScope();
            ctx.nullNarrowing.narrowToNonNull(narrowVarName);
        }
        node->getThenStatement()->accept(ctx.visitor);  // Will need delegation
        if (narrowInThen && !narrowVarName.empty())
        {
            ctx.nullNarrowing.exitScope();
        }
        ctx.variableTracker.endScope();
        ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

        if (node->getElseStatement()) {
            // Jump over else branch
            size_t endJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

            // Patch else jump
            ctx.emitter.patchJump(elseJump);

            // Compile else branch with its own scope
            ctx.variableTracker.beginScope();
            if (narrowInElse && !narrowVarName.empty())
            {
                ctx.nullNarrowing.enterScope();
                ctx.nullNarrowing.narrowToNonNull(narrowVarName);
            }
            node->getElseStatement()->accept(ctx.visitor);  // Will need delegation
            if (narrowInElse && !narrowVarName.empty())
            {
                ctx.nullNarrowing.exitScope();
            }
            ctx.variableTracker.endScope();
            ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

            // Patch end jump
            ctx.emitter.patchJump(endJump);
        } else {
            // No else branch, patch jump to end
            ctx.emitter.patchJump(elseJump);
        }

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileWhile(ast::WhileNode* node)
    {
        // Emit LOOP_START marker for optimization passes
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_START, node);

        size_t loopStart = ctx.program.getCurrentOffset();

        // Compile condition
        node->getCondition()->accept(ctx.visitor);  // Will need delegation

        // PHASE 4: Auto-unbox Bool objects to primitive bool
        value::ValueType conditionType = ctx.typeInference.inferExpressionType(node->getCondition());
        if (conditionType == value::ValueType::OBJECT)
        {
            std::string conditionClassName = ctx.typeInference.inferExpressionClassName(node->getCondition());
            if (conditionClassName == "Bool")
            {
                // Auto-unbox: call getValue() to get primitive bool
                size_t methodNameIndex = ctx.program.getConstantPool().addString("getValue");
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint64_t>(methodNameIndex),
                                             0u,  // 0 arguments
                                             node->getCondition());
            }
        }

        // Jump to end if condition is false
        size_t exitJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        // Enter loop context
        ctx.loopManager.enterLoop(loopStart);

        // Compile body with its own scope
        ctx.variableTracker.beginScope();
        node->getBody()->accept(ctx.visitor);  // Will need delegation
        ctx.variableTracker.endScope();
        ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

        // Jump back to start
        ctx.emitter.emitLoop(loopStart);

        // Emit LOOP_END marker
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_END, node);

        // Exit loop and patch jumps
        ctx.emitter.patchJump(exitJump);
        for (size_t breakJump : ctx.loopManager.getBreakJumps()) {
            ctx.emitter.patchJump(breakJump);
        }
        for (size_t continueJump : ctx.loopManager.getContinueJumps()) {
            ctx.program.patchJump(continueJump, static_cast<uint64_t>(loopStart));
        }
        ctx.loopManager.exitLoop();

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileDoWhile(ast::DoWhileNode* node)
    {
        // Emit LOOP_START marker for optimization passes
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_START, node);

        size_t loopStart = ctx.program.getCurrentOffset();

        // Enter loop context
        ctx.loopManager.enterLoop(loopStart);

        // Compile body with its own scope
        ctx.variableTracker.beginScope();
        node->getBody()->accept(ctx.visitor);  // Will need delegation
        ctx.variableTracker.endScope();
        ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

        // Compile condition
        node->getCondition()->accept(ctx.visitor);  // Will need delegation

        // PHASE 4: Auto-unbox Bool objects to primitive bool
        value::ValueType conditionType = ctx.typeInference.inferExpressionType(node->getCondition());
        if (conditionType == value::ValueType::OBJECT)
        {
            std::string conditionClassName = ctx.typeInference.inferExpressionClassName(node->getCondition());
            if (conditionClassName == "Bool")
            {
                // Auto-unbox: call getValue() to get primitive bool
                size_t methodNameIndex = ctx.program.getConstantPool().addString("getValue");
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint64_t>(methodNameIndex),
                                             0u,  // 0 arguments
                                             node->getCondition());
            }
        }

        // Jump back to start if condition is true
        size_t continueJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_TRUE);
        ctx.program.patchJump(continueJump, static_cast<uint64_t>(loopStart));

        // Emit LOOP_END marker
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_END, node);

        // Exit loop and patch jumps
        for (size_t breakJump : ctx.loopManager.getBreakJumps()) {
            ctx.emitter.patchJump(breakJump);
        }
        for (size_t contJump : ctx.loopManager.getContinueJumps()) {
            ctx.program.patchJump(contJump, static_cast<uint64_t>(loopStart));
        }
        ctx.loopManager.exitLoop();

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileFor(ast::ForNode* node)
    {
        ctx.variableTracker.beginScope();

        // Compile initializer
        if (node->getInitialization()) {
            node->getInitialization()->accept(ctx.visitor);  // Will need delegation
        }

        // Emit LOOP_START marker for optimization passes
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_START, node);

        size_t loopStart = ctx.program.getCurrentOffset();

        // Compile condition
        if (node->getCondition()) {
            node->getCondition()->accept(ctx.visitor);  // Will need delegation
        } else {
            // No condition means infinite loop
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_BOOL, 1, node);
        }

        size_t exitJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        // Jump over increment to body
        size_t bodyJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

        // Increment location (continue jumps here)
        size_t incrementStart = ctx.program.getCurrentOffset();
        if (node->getUpdate()) {
            node->getUpdate()->accept(ctx.visitor);  // Will need delegation
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);  // Discard increment result
        }

        // Jump back to condition
        ctx.emitter.emitLoop(loopStart);

        // Body starts here
        ctx.emitter.patchJump(bodyJump);

        // Enter loop context
        ctx.loopManager.enterLoop(loopStart, incrementStart);

        // Compile body
        if (node->getBody()) {
            node->getBody()->accept(ctx.visitor);  // Will need delegation
        }

        // Jump to increment
        ctx.emitter.emitLoop(incrementStart);

        // Emit LOOP_END marker
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_END, node);

        // Exit loop and patch jumps
        ctx.emitter.patchJump(exitJump);
        for (size_t breakJump : ctx.loopManager.getBreakJumps()) {
            ctx.emitter.patchJump(breakJump);
        }
        for (size_t continueJump : ctx.loopManager.getContinueJumps()) {
            ctx.program.patchJump(continueJump, static_cast<uint64_t>(incrementStart));
        }
        ctx.loopManager.exitLoop();

        ctx.variableTracker.endScope();
        ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileForEach(ast::ForEachNode* node)
    {
        std::string varName = node->getVariableName();
        value::ValueType varType = node->getVariableType();

        ctx.variableTracker.beginScope();

        // Evaluate collection
        node->getCollection()->accept(ctx.visitor);  // Will need delegation

        // Determine if we're iterating over an array or a collection
        // Check multiple indicators that suggest array type:
        // 1. Variable type is explicitly ARRAY (legacy)
        // 2. Collection is an array index access expression (arr[i])
        // 3. Collection is a method call to toArray() which returns an array
        // 4. Variable type info indicates array (has [] in declaration)

        // Check TypeInfo for array markers (element type is set means it's an array)
        const auto& varTypeInfo = node->getVariableTypeInfo();
        bool isArrayFromTypeInfo = (varType == value::ValueType::ARRAY);

        bool isArrayType = (isArrayFromTypeInfo ||
                           dynamic_cast<ast::IndexAccessNode*>(node->getCollection()) != nullptr);

        // Check if this is a toArray() method call
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

            // Try to find the variable in the local scope
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

        if (isArrayType) {
            // === ARRAY FAST PATH ===
            // Use counter-based iteration with ARRAY_GET (existing implementation)

            // Store collection in local
            ctx.variableTracker.declareLocal("__foreach_array__", value::ValueType::OBJECT, "");
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t arraySlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(arraySlot), node);

            // Get array length
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(arraySlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::ARRAY_LENGTH, node);

            // Store length in local
            ctx.variableTracker.declareLocal("__foreach_length__", value::ValueType::INT, "");
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t lengthSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(lengthSlot), node);

            // Initialize counter
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT,
                           static_cast<uint64_t>(ctx.program.getConstantPool().addInteger(0)), node);
            ctx.variableTracker.declareLocal("__foreach_counter__", value::ValueType::INT, "");
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t counterSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(counterSlot), node);

            // Emit LOOP_START marker for optimization passes
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

            // Store in loop variable
            ctx.variableTracker.declareLocal(varName, varType, "");
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t loopVarSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(loopVarSlot), node);

            // Enter loop context
            size_t continueTarget = ctx.program.getCurrentOffset();
            ctx.loopManager.enterLoop(loopStart, continueTarget);

            // Compile body
            if (node->getBody()) {
                node->getBody()->accept(ctx.visitor);  // Will need delegation
            }

            ctx.loopManager.exitLoop();

            // Increment counter
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(counterSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT,
                           static_cast<uint64_t>(ctx.program.getConstantPool().addInteger(1)), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::ADD, node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(counterSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

            // Jump back to loop start
            ctx.emitter.emitLoop(loopStart);

            // Emit LOOP_END marker
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_END, node);

            // Patch exit jump
            ctx.emitter.patchJump(exitJump);
        }
        else {
            // === ITERATOR PATH ===
            // Use GET_ITERATOR, ITERATOR_HAS_NEXT, ITERATOR_NEXT opcodes

            // Validate that collection implements Iterable<T> and element type matches loop variable
            // Skip validation for array types (they should have been caught by array path detection)
            std::string collectionClassName = ctx.typeInference.inferExpressionClassName(node->getCollection());
            if (collectionClassName.empty()) {
                collectionClassName = "Object";  // Fallback for unknown types
            }

            // Double-check: arrays should not reach this path, but if they do, skip validation
            bool skipValidation = (collectionClassName.find("[]") != std::string::npos);

            if (!skipValidation) {
                // Get loop variable type as string from TypeInfo
                const auto& varTypeInfo = node->getVariableTypeInfo();
                std::string loopVarTypeName;

                // For object types, use className (which may include generics like "ArrayList<Int>")
                // For primitive types, convert ValueType to string representation
                if (varTypeInfo.baseType == value::ValueType::OBJECT && !varTypeInfo.className.empty()) {
                    loopVarTypeName = varTypeInfo.className;
                } else {
                    // Use toString() method or convert baseType to string for primitives
                    loopVarTypeName = varTypeInfo.toString();
                }

                // Perform validation (will throw exception if invalid)
                try {
                    ctx.typeValidator.validateAndExtractIterableElementType(
                        collectionClassName,
                        loopVarTypeName,
                        node->getLocation()
                    );
                } catch (const errors::TypeException& e) {
                    // Re-throw with location information
                    throw errors::TypeException(e.what(), node->getLocation());
                }
            }

            // Collection is already on the stack from node->getCollection()->accept()
            // Call GET_ITERATOR to get the iterator object
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

            // Store iterator in local variable
            ctx.variableTracker.declareLocal("__foreach_iterator__", value::ValueType::OBJECT, "");
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t iteratorSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(iteratorSlot), node);

            // Emit LOOP_START marker
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

            // Store in loop variable
            ctx.variableTracker.declareLocal(varName, varType, "");
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
            size_t loopVarSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(loopVarSlot), node);

            // Enter loop context
            size_t continueTarget = ctx.program.getCurrentOffset();
            ctx.loopManager.enterLoop(loopStart, continueTarget);

            // Compile body
            if (node->getBody()) {
                node->getBody()->accept(ctx.visitor);  // Will need delegation
            }

            ctx.loopManager.exitLoop();

            // Jump back to loop start (hasNext check)
            ctx.emitter.emitLoop(loopStart);

            // Emit LOOP_END marker
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_END, node);

            // Patch exit jump
            ctx.emitter.patchJump(exitJump);

            // Cleanup: Close the iterator
            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(iteratorSlot), node);
            ctx.emitter.emitWithLocation(bytecode::OpCode::ITERATOR_CLOSE, node);
        }

        ctx.variableTracker.endScope();

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileSwitch(ast::SwitchNode* node)
    {
        ctx.switchManager.enterSwitch();

        // Compile switch expression
        node->getExpression()->accept(ctx.visitor);  // Will need delegation

        const auto& cases = node->getCases();
        std::vector<size_t> caseBodyStarts;
        size_t defaultBodyStart = SIZE_MAX;
        size_t defaultCaseIndex = SIZE_MAX;

        // First pass: generate comparisons
        for (size_t i = 0; i < cases.size(); ++i) {
            if (dynamic_cast<ast::DefaultCaseNode*>(cases[i].get())) {
                defaultCaseIndex = i;
                caseBodyStarts.push_back(SIZE_MAX);
            } else if (auto* regularCase = dynamic_cast<ast::CaseNode*>(cases[i].get())) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::DUP, regularCase);
                regularCase->getValue()->accept(ctx.visitor);  // Will need delegation
                ctx.emitter.emitWithLocation(bytecode::OpCode::EQ, regularCase);
                size_t jumpToCaseBody = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_TRUE);
                caseBodyStarts.push_back(jumpToCaseBody);
            }
        }

        size_t jumpToDefaultOrEnd = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

        // Second pass: compile case bodies
        for (size_t i = 0; i < cases.size(); ++i) {
            if (caseBodyStarts[i] != SIZE_MAX) {
                ctx.emitter.patchJump(caseBodyStarts[i]);
            }

            if (i == defaultCaseIndex) {
                defaultBodyStart = ctx.program.getCurrentOffset();
            }

            if (caseBodyStarts[i] != SIZE_MAX || i == defaultCaseIndex) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::POP, cases[i].get());
            }

            // Compile case statements
            if (auto* defaultCase = dynamic_cast<ast::DefaultCaseNode*>(cases[i].get())) {
                for (const auto& stmt : defaultCase->getStatements()) {
                    stmt->accept(ctx.visitor);  // Will need delegation
                    // Pop unused return values from function call expression statements
                    if (dynamic_cast<ast::nodes::functions::FunctionCallNode*>(stmt.get())) {
                        ctx.emitter.emitWithLocation(bytecode::OpCode::POP, stmt.get());
                    }
                }
            } else if (auto* regularCase = dynamic_cast<ast::CaseNode*>(cases[i].get())) {
                for (const auto& stmt : regularCase->getStatements()) {
                    stmt->accept(ctx.visitor);  // Will need delegation
                    // Pop unused return values from function call expression statements
                    if (dynamic_cast<ast::nodes::functions::FunctionCallNode*>(stmt.get())) {
                        ctx.emitter.emitWithLocation(bytecode::OpCode::POP, stmt.get());
                    }
                }
            }
        }

        size_t implicitEndJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

        // Patch jump to default or end
        ctx.emitter.patchJump(jumpToDefaultOrEnd);
        if (defaultBodyStart != SIZE_MAX) {
            ctx.emitter.emitWithLocation(bytecode::OpCode::JUMP, static_cast<uint64_t>(defaultBodyStart), node);
        } else {
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);
        }

        // Patch all break jumps
        ctx.emitter.patchJump(implicitEndJump);
        for (size_t breakJump : ctx.switchManager.getBreakJumps()) {
            ctx.emitter.patchJump(breakJump);
        }

        ctx.switchManager.exitSwitch();

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileCase(ast::CaseNode* node)
    {
        // Case nodes are handled by SwitchNode
        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileDefaultCase(ast::DefaultCaseNode* node)
    {
        // Default case nodes are handled by SwitchNode
        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileMatch(ast::MatchNode* node)
    {
        using namespace ast::nodes::statements;

        // 1. Compile match expression and store in a temp local
        node->getExpression()->accept(ctx.visitor);

        ctx.variableTracker.beginScope();
        ctx.variableTracker.declareLocal("$match_val", value::ValueType::VOID);
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
        size_t matchSlot = ctx.variableTracker.getNextLocalSlot() - 1;
        size_t relativeMatchSlot = matchSlot;
        if (ctx.functionFrameManager.isInFunction()) {
            relativeMatchSlot = matchSlot - ctx.functionFrameManager.currentFrame().localStartSlot;
        }
        ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL,
            static_cast<uint64_t>(relativeMatchSlot), node);

        const auto& cases = node->getCases();
        std::vector<size_t> endJumps;

        for (size_t i = 0; i < cases.size(); ++i) {
            if (auto* matchCase = dynamic_cast<MatchCaseNode*>(cases[i].get())) {
                PatternKind kind = matchCase->getPatternKind();

                if (kind == PatternKind::TYPE) {
                    // TYPE PATTERN: case TypeName varName ->
                    ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL,
                        static_cast<uint64_t>(relativeMatchSlot), matchCase);

                    size_t typeNameIndex = ctx.program.getConstantPool().addString(matchCase->getTypeName());
                    ctx.emitter.emitWithLocation(bytecode::OpCode::INSTANCEOF,
                        static_cast<uint64_t>(typeNameIndex), matchCase);

                    size_t jumpToNext = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

                    // Bind the variable in a new scope
                    ctx.variableTracker.beginScope();
                    ctx.variableTracker.declareLocal(
                        matchCase->getBindingName(),
                        value::ValueType::OBJECT,
                        matchCase->getTypeName()
                    );
                    ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
                    size_t bindSlot = ctx.variableTracker.getNextLocalSlot() - 1;
                    size_t relBindSlot = bindSlot;
                    if (ctx.functionFrameManager.isInFunction()) {
                        relBindSlot = bindSlot - ctx.functionFrameManager.currentFrame().localStartSlot;
                    }

                    // Load match value and store into binding variable
                    ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL,
                        static_cast<uint64_t>(relativeMatchSlot), matchCase);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL,
                        static_cast<uint64_t>(relBindSlot), matchCase);

                    // Compile body
                    matchCase->getBody()->accept(ctx.visitor);

                    ctx.variableTracker.endScope();

                    endJumps.push_back(ctx.emitter.emitJump(bytecode::OpCode::JUMP));
                    ctx.emitter.patchJump(jumpToNext);
                }
                else if (kind == PatternKind::VALUE) {
                    // VALUE PATTERN: case expr ->
                    ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL,
                        static_cast<uint64_t>(relativeMatchSlot), matchCase);

                    matchCase->getValueExpression()->accept(ctx.visitor);

                    ctx.emitter.emitWithLocation(bytecode::OpCode::EQ, matchCase);

                    size_t jumpToNext = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

                    matchCase->getBody()->accept(ctx.visitor);

                    endJumps.push_back(ctx.emitter.emitJump(bytecode::OpCode::JUMP));
                    ctx.emitter.patchJump(jumpToNext);
                }
                else if (kind == PatternKind::NULL_PATTERN) {
                    // NULL PATTERN: case null ->
                    ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL,
                        static_cast<uint64_t>(relativeMatchSlot), matchCase);

                    ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, matchCase);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::EQ, matchCase);

                    size_t jumpToNext = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

                    matchCase->getBody()->accept(ctx.visitor);

                    endJumps.push_back(ctx.emitter.emitJump(bytecode::OpCode::JUMP));
                    ctx.emitter.patchJump(jumpToNext);
                }
            }
            else if (auto* defaultCase = dynamic_cast<MatchDefaultNode*>(cases[i].get())) {
                // DEFAULT: default ->
                defaultCase->getBody()->accept(ctx.visitor);
                endJumps.push_back(ctx.emitter.emitJump(bytecode::OpCode::JUMP));
            }
        }

        // Patch all end jumps
        for (size_t jump : endJumps) {
            ctx.emitter.patchJump(jump);
        }

        ctx.variableTracker.endScope(); // end match value scope

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileBreak(ast::BreakNode* node)
    {
        // IMPORTANT: Check switch context first!
        // When a switch is nested in a loop, break should exit the switch, not the loop
        if (ctx.switchManager.isInSwitch()) {
            size_t breakJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
            ctx.switchManager.registerBreak(breakJump);
        } else if (ctx.loopManager.isInLoop()) {
            // Check if we're in a try block with a finally, but NOT already inside the finally block
            // If we're IN the finally block, we should break directly (not jump to finally again)
            if (ctx.exceptionManager.hasPendingFinally() && !ctx.exceptionManager.isInFinally()) {
                // We're in a try-finally - register break jump with exception manager
                // The finally block will create a trampoline that executes finally then breaks the loop
                size_t breakJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
                ctx.exceptionManager.registerBreakJump(breakJump);
            } else {
                // Normal break - register with loop manager
                size_t breakJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
                ctx.loopManager.registerBreak(breakJump);
            }
        } else {
            throw errors::ParseException("Break outside of loop or switch");
        }
        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileContinue(ast::ContinueNode* node)
    {
        if (!ctx.loopManager.isInLoop()) {
            throw errors::ParseException("Continue outside of loop");
        }

        // Check if we're in a try block with a finally, but NOT already inside the finally block
        // If we're IN the finally block, we should continue directly (not jump to finally again)
        if (ctx.exceptionManager.hasPendingFinally() && !ctx.exceptionManager.isInFinally()) {
            // We're in a try-finally - register continue jump with exception manager
            // The finally block will create a trampoline that executes finally then continues the loop
            size_t continueJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
            ctx.exceptionManager.registerContinueJump(continueJump);
        } else {
            // Normal continue - register with loop manager
            size_t continueJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
            ctx.loopManager.registerContinue(continueJump);
        }
        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileTry(ast::TryNode* node)
    {
        return tryHelper->compileTry(node);
    }

    value::Value ControlFlowCompiler::compileThrow(ast::ThrowNode* node)
    {
        // Compile the exception expression (should evaluate to an object)
        node->getException()->accept(ctx.visitor);

        // Emit THROW instruction with source location
        ctx.emitter.emitWithLocation(bytecode::OpCode::THROW, node);

        return std::monostate{};
    }
}
