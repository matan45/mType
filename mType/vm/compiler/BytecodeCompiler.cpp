#include "BytecodeCompiler.hpp"
#include "../../errors/ParseException.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/statements/BlockNode.hpp"
#include "../../ast/nodes/statements/DeclarationNode.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../ast/nodes/statements/IfNode.hpp"
#include "../../ast/nodes/statements/WhileNode.hpp"
#include "../../ast/nodes/statements/DoWhileNode.hpp"
#include "../../ast/nodes/statements/ForNode.hpp"
#include "../../ast/nodes/statements/ForEachNode.hpp"
#include "../../ast/nodes/statements/BreakNode.hpp"
#include "../../ast/nodes/statements/ContinueNode.hpp"
#include "../../ast/nodes/statements/SwitchNode.hpp"
#include "../../ast/nodes/statements/CaseNode.hpp"
#include "../../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../ast/nodes/expressions/FloatNode.hpp"
#include "../../ast/nodes/expressions/StringNode.hpp"
#include "../../ast/nodes/expressions/BoolNode.hpp"
#include "../../ast/nodes/expressions/NullNode.hpp"
#include "../../ast/nodes/expressions/VariableNode.hpp"
#include "../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../ast/nodes/functions/ReturnNode.hpp"
#include <stdexcept>

namespace vm::compiler
{
    BytecodeCompiler::BytecodeCompiler(std::shared_ptr<environment::Environment> env)
        : environment(env), program(), loopStack(), locals(), nextLocalSlot(0)
    {
    }

    bytecode::BytecodeProgram BytecodeCompiler::compile(ast::ASTNode* root)
    {
        if (!root) {
            throw std::runtime_error("Cannot compile null AST root");
        }

        // Visit the root node to generate bytecode
        root->accept(*this);

        // Emit halt instruction
        program.emit(bytecode::OpCode::HALT);

        return std::move(program);
    }

    // ==================== Helper Methods ====================

    void BytecodeCompiler::emitWithLocation(bytecode::OpCode opcode, ast::ASTNode* node)
    {
        size_t offset = program.getCurrentOffset();
        program.emit(opcode);

        if (node) {
            const auto& loc = node->getLocation();
            program.addSourceLocation(offset, loc.getLine(), loc.getColumn(), loc.getFilename());
        }
    }

    void BytecodeCompiler::emitWithLocation(bytecode::OpCode opcode, uint32_t operand, ast::ASTNode* node)
    {
        size_t offset = program.getCurrentOffset();
        program.emit(opcode, operand);

        if (node) {
            const auto& loc = node->getLocation();
            program.addSourceLocation(offset, loc.getLine(), loc.getColumn(), loc.getFilename());
        }
    }

    size_t BytecodeCompiler::emitJump(bytecode::OpCode jumpOp)
    {
        program.emit(jumpOp, 0xFFFFFFFF);  // Placeholder offset
        return program.getCurrentOffset() - 1;
    }

    void BytecodeCompiler::patchJump(size_t offset)
    {
        size_t jumpTarget = program.getCurrentOffset();
        program.patchJump(offset, static_cast<uint32_t>(jumpTarget));
    }

    void BytecodeCompiler::emitLoop(size_t loopStart)
    {
        program.emit(bytecode::OpCode::JUMP_BACK, static_cast<uint32_t>(loopStart));
    }

    bytecode::OpCode BytecodeCompiler::getBinaryOpCode(token::TokenType op, bool typeSpecialized)
    {
        switch (op) {
            case token::TokenType::PLUS:
                return typeSpecialized ? bytecode::OpCode::ADD_INT : bytecode::OpCode::ADD;
            case token::TokenType::MINUS:
                return typeSpecialized ? bytecode::OpCode::SUB_INT : bytecode::OpCode::SUB;
            case token::TokenType::MULTIPLY:
                return typeSpecialized ? bytecode::OpCode::MUL_INT : bytecode::OpCode::MUL;
            case token::TokenType::DIVIDE:
                return typeSpecialized ? bytecode::OpCode::DIV_INT : bytecode::OpCode::DIV;
            case token::TokenType::MODULO:
                return bytecode::OpCode::MOD;
            case token::TokenType::EQUALS:
                return bytecode::OpCode::EQ;
            case token::TokenType::NOT_EQUALS:
                return bytecode::OpCode::NE;
            case token::TokenType::LESS:
                return bytecode::OpCode::LT;
            case token::TokenType::GREATER:
                return bytecode::OpCode::GT;
            case token::TokenType::LESS_EQUALS:
                return bytecode::OpCode::LE;
            case token::TokenType::GREATER_EQUALS:
                return bytecode::OpCode::GE;
            case token::TokenType::AND:
                return bytecode::OpCode::AND;
            case token::TokenType::OR:
                return bytecode::OpCode::OR;
            default:
                throw errors::ParseException("Unsupported binary operator");
        }
    }

    bytecode::OpCode BytecodeCompiler::getUnaryOpCode(token::TokenType op)
    {
        switch (op) {
            case token::TokenType::MINUS:
                return bytecode::OpCode::NEG;
            case token::TokenType::NOT:
                return bytecode::OpCode::NOT;
            case token::TokenType::INCREMENT:
                return bytecode::OpCode::INC;
            case token::TokenType::DECREMENT:
                return bytecode::OpCode::DEC;
            default:
                throw errors::ParseException("Unsupported unary operator");
        }
    }

    size_t BytecodeCompiler::resolveLocal(const std::string& name)
    {
        for (size_t i = locals.size(); i > 0; --i) {
            if (locals[i - 1].name == name) {
                return locals[i - 1].slot;
            }
        }
        return SIZE_MAX;  // Not found
    }

    void BytecodeCompiler::beginScope()
    {
        // Mark the start of a new scope (can track for cleanup)
    }

    void BytecodeCompiler::endScope()
    {
        // Pop locals that went out of scope
        while (!locals.empty() && locals.back().slot >= nextLocalSlot) {
            locals.pop_back();
            emitWithLocation(bytecode::OpCode::POP, nullptr);
        }
    }

    void BytecodeCompiler::enterLoop(size_t loopStart)
    {
        LoopContext ctx;
        ctx.loopStart = loopStart;
        loopStack.push_back(ctx);
    }

    void BytecodeCompiler::exitLoop()
    {
        if (loopStack.empty()) {
            throw errors::RuntimeException("Loop stack underflow");
        }

        LoopContext& ctx = loopStack.back();

        // Patch all break jumps to current position (after loop)
        for (size_t breakJump : ctx.breakJumps) {
            patchJump(breakJump);
        }

        // Patch all continue jumps to loop start
        for (size_t continueJump : ctx.continueJumps) {
            program.patchJump(continueJump, static_cast<uint32_t>(ctx.loopStart));
        }

        loopStack.pop_back();
    }

    BytecodeCompiler::LoopContext& BytecodeCompiler::currentLoop()
    {
        if (loopStack.empty()) {
            throw errors::RuntimeException("Not in a loop context");
        }
        return loopStack.back();
    }

    // ==================== AST Visitor Implementations ====================

    value::Value BytecodeCompiler::visitProgramNode(ast::ProgramNode* node)
    {
        const auto& statements = node->getStatements();
        for (auto& stmt : statements) {
            stmt->accept(*this);
        }
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitBlockNode(ast::BlockNode* node)
    {
        beginScope();
        const auto& statements = node->getStatements();
        for (auto& stmt : statements) {
            stmt->accept(*this);
        }
        endScope();
        return std::monostate{};
    }

    // ==================== Literals ====================

    value::Value BytecodeCompiler::visitIntegerNode(ast::IntegerNode* node)
    {
        size_t index = program.getConstantPool().addInteger(node->getValue());
        emitWithLocation(bytecode::OpCode::PUSH_INT, static_cast<uint32_t>(index), node);
    }

    value::Value BytecodeCompiler::visitFloatNode(ast::FloatNode* node)
    {
        size_t index = program.getConstantPool().addFloat(node->getValue());
        emitWithLocation(bytecode::OpCode::PUSH_FLOAT, static_cast<uint32_t>(index), node);
    }

    value::Value BytecodeCompiler::visitStringNode(ast::StringNode* node)
    {
        size_t index = program.getConstantPool().addString(node->getValue());
        emitWithLocation(bytecode::OpCode::PUSH_STRING, static_cast<uint32_t>(index), node);
    }

    value::Value BytecodeCompiler::visitBoolNode(ast::BoolNode* node)
    {
        emitWithLocation(bytecode::OpCode::PUSH_BOOL, node->getValue() ? 1 : 0, node);
    }

    value::Value BytecodeCompiler::visitNullNode(ast::NullNode* node)
    {
        emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
    }

    // ==================== Variables ====================

    value::Value BytecodeCompiler::visitVariableNode(ast::VariableNode* node)
    {
        std::string name = node->getName();
        size_t localSlot = resolveLocal(name);

        if (localSlot != SIZE_MAX) {
            // Local variable
            emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(localSlot), node);
        } else {
            // Global variable
            size_t nameIndex = program.getConstantPool().addString(name);
            emitWithLocation(bytecode::OpCode::LOAD_VAR, static_cast<uint32_t>(nameIndex), node);
        }
    }

    value::Value BytecodeCompiler::visitDeclarationNode(ast::DeclarationNode* node)
    {
        // Compile the initializer (if any)
        auto* initializer = node->getInitializer();
        if (initializer) {
            initializer->accept(*this);
        } else {
            emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
        }

        // Store in variable
        std::string name = node->getVariableName();
        value::ValueType valueType = node->getType();

        // Convert ValueType enum to string (simplified - you may want a proper converter)
        std::string typeStr = "auto";  // Default type string

        size_t nameIndex = program.getConstantPool().addString(name);
        size_t typeIndex = program.getConstantPool().addString(typeStr);

        program.emit(bytecode::OpCode::DECLARE_VAR,
                     static_cast<uint32_t>(nameIndex),
                     static_cast<uint32_t>(typeIndex));

        // Track as local if in a function scope
        if (!loopStack.empty() || nextLocalSlot > 0) {
            LocalVariable local;
            local.name = name;
            local.slot = nextLocalSlot++;
            locals.push_back(local);
        }
    }

    value::Value BytecodeCompiler::visitAssignmentNode(ast::AssignmentNode* node)
    {
        // Compile the value
        node->getValue()->accept(*this);

        std::string name = node->getVariableName();
        size_t localSlot = resolveLocal(name);

        if (localSlot != SIZE_MAX) {
            // Local variable
            emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(localSlot), node);
        } else {
            // Global variable
            size_t nameIndex = program.getConstantPool().addString(name);
            emitWithLocation(bytecode::OpCode::STORE_VAR, static_cast<uint32_t>(nameIndex), node);
        }
    }

    // ==================== Operators ====================

    value::Value BytecodeCompiler::visitBinaryOpNode(ast::BinaryOpNode* node)
    {
        // Compile left operand
        node->getLeft()->accept(*this);

        // Compile right operand
        node->getRight()->accept(*this);

        // Emit operation
        bytecode::OpCode opcode = getBinaryOpCode(node->getOperator(), false);
        emitWithLocation(opcode, node);
    }

    value::Value BytecodeCompiler::visitUnaryOpNode(ast::UnaryOpNode* node)
    {
        // Compile operand
        node->getOperand()->accept(*this);

        // Emit operation
        bytecode::OpCode opcode = getUnaryOpCode(node->getOperator());
        emitWithLocation(opcode, node);
    }

    value::Value BytecodeCompiler::visitTernaryOpNode(ast::TernaryOpNode* node)
    {
        // Compile condition
        node->getCondition()->accept(*this);

        // Jump to false branch if condition is false
        size_t falseJump = emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        // Compile true branch
        node->getTrueExpression()->accept(*this);
        size_t endJump = emitJump(bytecode::OpCode::JUMP);

        // Compile false branch
        patchJump(falseJump);
        node->getFalseExpression()->accept(*this);

        // Patch end jump
        patchJump(endJump);
    }

    // ==================== Control Flow ====================

    value::Value BytecodeCompiler::visitIfNode(ast::IfNode* node)
    {
        // Compile condition
        node->getCondition()->accept(*this);

        // Jump to else/end if condition is false
        size_t elseJump = emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        // Compile then branch
        node->getThenStatement()->accept(*this);

        auto* elseStmt = node->getElseStatement();
        if (elseStmt) {
            // Jump over else branch
            size_t endJump = emitJump(bytecode::OpCode::JUMP);

            // Compile else branch
            patchJump(elseJump);
            elseStmt->accept(*this);

            // Patch end jump
            patchJump(endJump);
        } else {
            // No else branch, just patch the jump
            patchJump(elseJump);
        }
    }

    value::Value BytecodeCompiler::visitWhileNode(ast::WhileNode* node)
    {
        size_t loopStart = program.getCurrentOffset();
        enterLoop(loopStart);

        // Compile condition
        node->getCondition()->accept(*this);

        // Jump to end if condition is false
        size_t exitJump = emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        // Compile body
        node->getBody()->accept(*this);

        // Jump back to loop start
        emitLoop(loopStart);

        // Patch exit jump
        patchJump(exitJump);

        exitLoop();
    }

    value::Value BytecodeCompiler::visitDoWhileNode(ast::DoWhileNode* node)
    {
        size_t loopStart = program.getCurrentOffset();
        enterLoop(loopStart);

        // Compile body
        node->getBody()->accept(*this);

        // Compile condition
        node->getCondition()->accept(*this);

        // Jump back to loop start if condition is true
        program.emit(bytecode::OpCode::JUMP_IF_TRUE, static_cast<uint32_t>(loopStart));

        exitLoop();
    }

    value::Value BytecodeCompiler::visitForNode(ast::ForNode* node)
    {
        beginScope();

        // Compile initializer
        auto* init = node->getInitialization();
        if (init) {
            init->accept(*this);
        }

        size_t loopStart = program.getCurrentOffset();
        enterLoop(loopStart);

        // Compile condition
        size_t exitJump = SIZE_MAX;
        auto* cond = node->getCondition();
        if (cond) {
            cond->accept(*this);
            exitJump = emitJump(bytecode::OpCode::JUMP_IF_FALSE);
        }

        // Compile body
        node->getBody()->accept(*this);

        // Compile increment
        auto* update = node->getUpdate();
        if (update) {
            update->accept(*this);
            emitWithLocation(bytecode::OpCode::POP, nullptr);  // Discard increment result
        }

        // Jump back to loop start
        emitLoop(loopStart);

        // Patch exit jump
        if (exitJump != SIZE_MAX) {
            patchJump(exitJump);
        }

        exitLoop();
        endScope();
    }

    value::Value BytecodeCompiler::visitBreakNode(ast::BreakNode* node)
    {
        if (loopStack.empty()) {
            throw errors::ParseException("Break outside of loop");
        }

        size_t breakJump = emitJump(bytecode::OpCode::JUMP);
        currentLoop().breakJumps.push_back(breakJump);
    }

    value::Value BytecodeCompiler::visitContinueNode(ast::ContinueNode* node)
    {
        if (loopStack.empty()) {
            throw errors::ParseException("Continue outside of loop");
        }

        size_t continueJump = emitJump(bytecode::OpCode::JUMP);
        currentLoop().continueJumps.push_back(continueJump);
    }

    // ==================== Placeholder Implementations ====================
    // These will be implemented in subsequent phases

    value::Value BytecodeCompiler::visitForEachNode(ast::ForEachNode* node)
    {
        throw std::runtime_error("ForEach compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitSwitchNode(ast::SwitchNode* node)
    {
        throw std::runtime_error("Switch compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitCaseNode(ast::CaseNode* node)
    {
        throw std::runtime_error("Case compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitDefaultCaseNode(ast::DefaultCaseNode* node)
    {
        throw std::runtime_error("DefaultCase compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitFunctionNode(ast::FunctionNode* node)
    {
        throw std::runtime_error("Function compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitFunctionCallNode(ast::FunctionCallNode* node)
    {
        throw std::runtime_error("FunctionCall compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitReturnNode(ast::ReturnNode* node)
    {
        auto* returnValue = node->getReturnValue();
        if (returnValue) {
            returnValue->accept(*this);
            emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
        } else {
            emitWithLocation(bytecode::OpCode::RETURN, node);
        }
    }

    value::Value BytecodeCompiler::visitNativeFunctionNode(ast::NativeFunctionNode* node)
    {
        throw std::runtime_error("NativeFunction compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitClassNode(ast::ClassNode* node)
    {
        throw std::runtime_error("Class compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitMethodNode(ast::MethodNode* node)
    {
        throw std::runtime_error("Method compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitConstructorNode(ast::ConstructorNode* node)
    {
        throw std::runtime_error("Constructor compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitFieldNode(ast::FieldNode* node)
    {
        throw std::runtime_error("Field compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitNewNode(ast::NewNode* node)
    {
        throw std::runtime_error("New compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitMemberAccessNode(ast::MemberAccessNode* node)
    {
        throw std::runtime_error("MemberAccess compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitMemberAssignmentNode(ast::MemberAssignmentNode* node)
    {
        throw std::runtime_error("MemberAssignment compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitMethodCallNode(ast::MethodCallNode* node)
    {
        throw std::runtime_error("MethodCall compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitSuperConstructorCallNode(ast::SuperConstructorCallNode* node)
    {
        throw std::runtime_error("SuperConstructorCall compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitSuperMethodCallNode(ast::SuperMethodCallNode* node)
    {
        throw std::runtime_error("SuperMethodCall compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitInterfaceNode(ast::InterfaceNode* node)
    {
        throw std::runtime_error("Interface compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitArrayCreationNode(ast::ArrayCreationNode* node)
    {
        throw std::runtime_error("ArrayCreation compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitArrayLiteralNode(ast::ArrayLiteralNode* node)
    {
        throw std::runtime_error("ArrayLiteral compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitIndexAccessNode(ast::IndexAccessNode* node)
    {
        throw std::runtime_error("IndexAccess compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitIndexAssignmentNode(ast::IndexAssignmentNode* node)
    {
        throw std::runtime_error("IndexAssignment compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitLambdaNode(ast::LambdaNode* node)
    {
        throw std::runtime_error("Lambda compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitCastExpression(ast::CastExpression* node)
    {
        throw std::runtime_error("Cast compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitInstanceOfExpression(ast::InstanceOfExpression* node)
    {
        throw std::runtime_error("InstanceOf compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitImportNode(ast::ImportNode* node)
    {
        throw std::runtime_error("Import compilation not yet implemented");
    }

} // namespace vm::compiler
