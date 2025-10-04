#include "BytecodeCompiler.hpp"
#include "../../errors/ParseException.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../evaluator/utils/ValueConverter.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/statements/BlockNode.hpp"
#include "../../ast/nodes/statements/DeclarationNode.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../ast/nodes/statements/IfNode.hpp"
#include "../../ast/nodes/statements/WhileNode.hpp"
#include "../../ast/nodes/statements/DoWhileNode.hpp"
#include "../../ast/nodes/statements/ForNode.hpp"
#include "../../ast/nodes/statements/ForEachNode.hpp"
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
#include "../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../ast/nodes/classes/MethodNode.hpp"
#include "../../ast/nodes/classes/FieldNode.hpp"
#include "../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../ast/nodes/classes/NewNode.hpp"
#include "../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../ast/nodes/classes/InterfaceNode.hpp"
#include "../../ast/nodes/classes/SuperConstructorCallNode.hpp"
#include "../../ast/nodes/classes/SuperMethodCallNode.hpp"
#include "../../ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../ast/nodes/statements/IndexAssignmentNode.hpp"
#include "../../ast/nodes/expressions/LambdaNode.hpp"
#include "../../ast/nodes/expressions/CastExpression.hpp"
#include "../../ast/nodes/expressions/InstanceOfExpression.hpp"
#include "../../ast/nodes/statements/ImportNode.hpp"
#include "../../value/LambdaValue.hpp"
#include <stdexcept>
#include <iostream>

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

        // First, register all classes in the environment for bytecode execution
        registerClassesForBytecode(root);

        // Visit the root node to generate bytecode
        root->accept(*this);

        // Emit halt instruction
        program.emit(bytecode::OpCode::HALT);

        return std::move(program);
    }

    void BytecodeCompiler::registerClassesForBytecode(ast::ASTNode* node)
    {
        if (!node) return;

        // Check if this node is a ClassNode
        if (auto classNode = dynamic_cast<ast::ClassNode*>(node))
        {
            std::string className = classNode->getClassName();

            // Register class for bytecode WITHOUT using evaluator
            // This only registers the class structure - bytecode handles all initialization

            // Check if class already registered
            if (environment->findClass(className)) {
                return; // Already registered, skip
            }

            // Create class definition programmatically
            auto classRegistry = environment->getClassRegistry();
            if (!classRegistry) {
                throw std::runtime_error("Class registry not available");
            }

            // Create a new class definition
            auto classDef = std::make_shared<runtimeTypes::klass::ClassDefinition>(className);

            // Handle parent class
            if (classNode->hasParentClass()) {
                classDef->setParentClassName(classNode->getParentClassName());
            }

            // Register constructors
            for (const auto& constructor : classNode->getConstructors()) {
                if (auto* ctorNode = dynamic_cast<ast::nodes::classes::ConstructorNode*>(constructor.get())) {
                    auto ctorDef = std::make_shared<runtimeTypes::klass::ConstructorDefinition>(
                        ctorNode->getParametersWithTypes(),
                        ctorNode->getBody()
                    );
                    classDef->addConstructor(ctorDef);
                }
            }

            // Register methods
            for (const auto& method : classNode->getMethods()) {
                if (auto* methodNode = dynamic_cast<ast::nodes::classes::MethodNode*>(method.get())) {
                    auto methodDef = std::make_shared<runtimeTypes::klass::MethodDefinition>(
                        methodNode->getName(),
                        methodNode->getReturnType(),
                        methodNode->getParameters(),
                        std::vector<std::pair<std::string, value::Value>>{},
                        methodNode->getBody(),
                        methodNode->getIsStatic()
                    );

                    if (methodNode->getIsStatic()) {
                        classDef->addStaticMethod(methodNode->getName(), methodDef);
                    } else {
                        classDef->addMethod(methodDef);
                    }
                }
            }

            // Register fields (but don't initialize them - bytecode will do that)
            for (const auto& field : classNode->getFields()) {
                if (auto* fieldNode = dynamic_cast<ast::nodes::classes::FieldNode*>(field.get())) {
                    auto fieldDef = std::make_shared<runtimeTypes::klass::FieldDefinition>(
                        fieldNode->getName(),
                        fieldNode->getType(),
                        value::Value{},  // Empty value - bytecode will initialize
                        fieldNode->getIsStatic(),
                        fieldNode->getIsFinal(),
                        fieldNode->getAccessModifier()  // Pass access modifier from AST
                    );

                    if (fieldNode->getIsStatic()) {
                        classDef->addStaticField(fieldNode->getName(), fieldDef);
                    } else {
                        classDef->addInstanceField(fieldNode->getName(), fieldDef);
                    }
                }
            }

            // Register the class
            classRegistry->registerClass(className, classDef);

            return; // No need to traverse children of ClassNode
        }

        // Recursively process child nodes
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                registerClassesForBytecode(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                registerClassesForBytecode(statement.get());
            }
        }
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
        // Only search locals that belong to the current function frame
        size_t startSlot = functionFrameStack.empty() ? 0 : functionFrameStack.back().localStartSlot;

        for (size_t i = locals.size(); i > 0; --i) {
            const LocalVariable& local = locals[i - 1];
            // Only consider locals from the current function (slot >= startSlot)
            if (local.slot >= startSlot && local.name == name) {
                return local.slot - startSlot;  // Return relative slot within this frame
            }
        }
        return SIZE_MAX;  // Not found
    }

    void BytecodeCompiler::beginScope()
    {
        currentScopeDepth++;
    }

    void BytecodeCompiler::endScope()
    {
        currentScopeDepth--;

        // Pop locals that went out of scope
        while (!locals.empty() && locals.back().scopeDepth > currentScopeDepth) {
            locals.pop_back();
            nextLocalSlot--;
            emitWithLocation(bytecode::OpCode::POP, nullptr);
        }
    }

    void BytecodeCompiler::enterFunctionFrame(const std::string& returnType)
    {
        FunctionFrame frame;
        frame.localStartSlot = nextLocalSlot;
        frame.scopeDepthStart = currentScopeDepth;
        frame.returnType = returnType;
        functionFrameStack.push_back(frame);
    }

    void BytecodeCompiler::exitFunctionFrame()
    {
        if (!functionFrameStack.empty()) {
            FunctionFrame frame = functionFrameStack.back();
            functionFrameStack.pop_back();

            // Clean up all locals from this function frame
            while (!locals.empty() && locals.back().slot >= frame.localStartSlot) {
                locals.pop_back();
            }

            // Reset slot counter and scope depth
            nextLocalSlot = frame.localStartSlot;
            currentScopeDepth = frame.scopeDepthStart;
        }
    }

    size_t BytecodeCompiler::getLocalCount() const
    {
        if (functionFrameStack.empty()) {
            return 0;
        }
        return nextLocalSlot - functionFrameStack.back().localStartSlot;
    }

    void BytecodeCompiler::enterLoop(size_t loopStart, size_t continueTarget)
    {
        LoopContext ctx;
        ctx.loopStart = loopStart;
        ctx.continueTarget = (continueTarget == SIZE_MAX) ? loopStart : continueTarget;
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

        // Patch all continue jumps to continue target (for loops: increment, others: loop start)
        for (size_t continueJump : ctx.continueJumps) {
            program.patchJump(continueJump, static_cast<uint32_t>(ctx.continueTarget));
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
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitFloatNode(ast::FloatNode* node)
    {
        size_t index = program.getConstantPool().addFloat(node->getValue());
        emitWithLocation(bytecode::OpCode::PUSH_FLOAT, static_cast<uint32_t>(index), node);
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitStringNode(ast::StringNode* node)
    {
        size_t index = program.getConstantPool().addString(node->getValue());
        emitWithLocation(bytecode::OpCode::PUSH_STRING, static_cast<uint32_t>(index), node);
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitBoolNode(ast::BoolNode* node)
    {
        emitWithLocation(bytecode::OpCode::PUSH_BOOL, node->getValue() ? 1 : 0, node);
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitNullNode(ast::NullNode* node)
    {
        emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
        return std::monostate{};
    }

    // ==================== Variables ====================

    value::Value BytecodeCompiler::visitVariableNode(ast::VariableNode* node)
    {
        std::string name = node->getName();

        // Check if this is a qualified static access (contains ::)
        if (name.find("::") != std::string::npos) {
            // This is a static field/variable access (e.g., MyClass::count)
            size_t nameIndex = program.getConstantPool().addString(name);
            emitWithLocation(bytecode::OpCode::GET_STATIC, static_cast<uint32_t>(nameIndex), node);
            return std::monostate{};
        }

        // Check if we're in a class context (instance or static method)
        if (currentClassNode) {
            // Check if it's a field of the current class
            for (const auto& field : currentClassNode->getFields()) {
                if (auto* fieldNode = dynamic_cast<ast::FieldNode*>(field.get())) {
                    if (fieldNode->getName() == name) {
                        // Found a field with this name
                        if (fieldNode->getIsStatic()) {
                            // Static field - use GET_STATIC with qualified name
                            std::string qualifiedName = currentClassNode->getClassName() + "::" + name;
                            size_t nameIndex = program.getConstantPool().addString(qualifiedName);
                            emitWithLocation(bytecode::OpCode::GET_STATIC, static_cast<uint32_t>(nameIndex), node);
                            return std::monostate{};
                        } else if (inInstanceMethod) {
                            // Instance field - use GET_FIELD with 'this'
                            // Load 'this' from local slot 0 (first parameter in instance methods)
                            emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0, node);

                            // Get field from 'this'
                            size_t fieldNameIndex = program.getConstantPool().addString(name);
                            emitWithLocation(bytecode::OpCode::GET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);
                            return std::monostate{};
                        }
                        // If we're in a static method trying to access instance field, fall through to error
                        break;
                    }
                }
            }
        }

        // Check if this is a local variable
        size_t localSlot = resolveLocal(name);
        if (localSlot != SIZE_MAX) {
            // This is a local variable - use LOAD_LOCAL with slot index
            emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(localSlot), node);
            return std::monostate{};
        }

        // Global variable lookup
        size_t nameIndex = program.getConstantPool().addString(name);
        emitWithLocation(bytecode::OpCode::LOAD_VAR, static_cast<uint32_t>(nameIndex), node);

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitDeclarationNode(ast::DeclarationNode* node)
    {
        std::string name = node->getVariableName();

        // Compile the initializer (if any)
        auto* initializer = node->getInitializer();
        if (initializer) {
            initializer->accept(*this);
        } else {
            emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
        }

        // Store in variable
        value::ValueType valueType = node->getType();

        // Track local variable if we're in a function
        if (!functionFrameStack.empty() && currentScopeDepth > 0) {
            // This is a local variable - add to locals tracking
            LocalVariable local;
            local.name = name;
            local.slot = nextLocalSlot++;
            local.scopeDepth = currentScopeDepth;
            locals.push_back(local);

            // The value is already on the stack from the initializer
            // For stack-based locals, we don't emit DECLARE_VAR
            // The value remains on the stack at the slot position
            return std::monostate{};
        }

        // This is a global variable - use DECLARE_VAR
        std::string typeStr = "auto";  // Default type string
        size_t nameIndex = program.getConstantPool().addString(name);
        size_t typeIndex = program.getConstantPool().addString(typeStr);
        uint32_t isFinal = node->isFinal() ? 1 : 0;

        program.emit(bytecode::OpCode::DECLARE_VAR,
                     std::vector<uint32_t>{
                         static_cast<uint32_t>(nameIndex),
                         static_cast<uint32_t>(typeIndex),
                         isFinal
                     });

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitAssignmentNode(ast::AssignmentNode* node)
    {
        std::string name = node->getVariableName();
        value::ValueType varType = node->getVariableType();

        // Compile the value
        node->getValue()->accept(*this);

        // Check if this is a declaration (has a type) or pure assignment (type is VOID)
        if (varType != value::ValueType::VOID) {
            // This is a declaration with initializer (e.g., "int x = 5;")

            // Track local variable if we're in a function
            if (!functionFrameStack.empty() && currentScopeDepth > 0) {
                // This is a local variable - add to locals tracking
                LocalVariable local;
                local.name = name;
                local.slot = nextLocalSlot++;
                local.scopeDepth = currentScopeDepth;
                locals.push_back(local);

                // Store the value at the local's slot position and keep a copy on stack
                emitWithLocation(bytecode::OpCode::DUP, node);
                emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(local.slot), node);
                return std::monostate{};
            }

            // This is a global variable - use DECLARE_VAR
            std::string typeStr = "auto";
            size_t nameIndex = program.getConstantPool().addString(name);
            size_t typeIndex = program.getConstantPool().addString(typeStr);
            uint32_t isFinal = node->getIsFinal() ? 1 : 0;

            program.emit(bytecode::OpCode::DECLARE_VAR,
                         std::vector<uint32_t>{
                             static_cast<uint32_t>(nameIndex),
                             static_cast<uint32_t>(typeIndex),
                             isFinal
                         });
        } else {
            // This is a pure assignment (e.g., "x = 5;")

            // Check if this is a qualified static assignment (contains ::)
            if (name.find("::") != std::string::npos) {
                // This is a static field assignment (e.g., MyClass::count = value)
                size_t nameIndex = program.getConstantPool().addString(name);
                emitWithLocation(bytecode::OpCode::SET_STATIC, static_cast<uint32_t>(nameIndex), node);
                return std::monostate{};
            }

            // Check if we're in a class context and this is a field assignment
            if (currentClassNode) {
                for (const auto& field : currentClassNode->getFields()) {
                    if (auto* fieldNode = dynamic_cast<ast::FieldNode*>(field.get())) {
                        if (fieldNode->getName() == name) {
                            // Found a field with this name
                            if (fieldNode->getIsStatic()) {
                                // Static field assignment - use SET_STATIC with qualified name
                                std::string qualifiedName = currentClassNode->getClassName() + "::" + name;
                                size_t nameIndex = program.getConstantPool().addString(qualifiedName);
                                emitWithLocation(bytecode::OpCode::SET_STATIC, static_cast<uint32_t>(nameIndex), node);
                                return std::monostate{};
                            } else if (inInstanceMethod) {
                                // Instance field assignment - use SET_FIELD with 'this'
                                // Load 'this' from local slot 0
                                emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0, node);

                                // Swap so stack is: this, value
                                program.emit(bytecode::OpCode::SWAP);

                                // Set field
                                size_t fieldNameIndex = program.getConstantPool().addString(name);
                                emitWithLocation(bytecode::OpCode::SET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);

                                return std::monostate{};
                            }
                            // If we're in a static method trying to assign to instance field, fall through to error
                            break;
                        }
                    }
                }
            }

            // Check if this is a local variable assignment
            size_t localSlot = resolveLocal(name);
            if (localSlot != SIZE_MAX) {
                // This is a local variable - use STORE_LOCAL with slot index
                // Duplicate value for assignment expression result (assignment returns the assigned value)
                emitWithLocation(bytecode::OpCode::DUP, node);
                emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(localSlot), node);
                return std::monostate{};
            }

            // Global variable assignment
            size_t nameIndex = program.getConstantPool().addString(name);
            emitWithLocation(bytecode::OpCode::STORE_VAR, static_cast<uint32_t>(nameIndex), node);
        }

        return std::monostate{};
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
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitUnaryOpNode(ast::UnaryOpNode* node)
    {
        auto op = node->getOperator();

        // Handle increment/decrement specially (they modify variables)
        if (op == token::TokenType::INCREMENT || op == token::TokenType::DECREMENT) {
            // Check if operand is a variable
            if (auto* varNode = dynamic_cast<ast::VariableNode*>(node->getOperand())) {
                std::string varName = varNode->getName();

                // Check if this is a local variable
                size_t localSlot = resolveLocal(varName);
                bool isLocal = (localSlot != SIZE_MAX);

                // Load current value
                if (isLocal) {
                    emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(localSlot), node);
                } else {
                    size_t nameIndex = program.getConstantPool().addString(varName);
                    program.emit(bytecode::OpCode::LOAD_VAR, static_cast<uint32_t>(nameIndex));
                }

                // For postfix (i++): duplicate value before incrementing (return original)
                // For prefix (++i): increment then duplicate (return incremented)
                bool isPostfix = (node->getPosition() == ast::nodes::expressions::UnaryPosition::POSTFIX);

                if (isPostfix) {
                    emitWithLocation(bytecode::OpCode::DUP, node);  // Save original value
                }

                // Increment or decrement
                bytecode::OpCode opcode = (op == token::TokenType::INCREMENT)
                    ? bytecode::OpCode::INC
                    : bytecode::OpCode::DEC;
                emitWithLocation(opcode, node);

                if (!isPostfix) {
                    emitWithLocation(bytecode::OpCode::DUP, node);  // Save incremented value
                }

                // Store updated value back to variable (consumes one value from stack)
                if (isLocal) {
                    emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(localSlot), node);
                } else {
                    size_t nameIndex = program.getConstantPool().addString(varName);
                    program.emit(bytecode::OpCode::STORE_VAR, static_cast<uint32_t>(nameIndex));
                }

                // Stack now has either original (postfix) or incremented (prefix) value
                return std::monostate{};
            }
        }

        // For other unary operators (-, !, etc.)
        node->getOperand()->accept(*this);
        bytecode::OpCode opcode = getUnaryOpCode(op);
        emitWithLocation(opcode, node);
        return std::monostate{};
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
        return std::monostate{};
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
        return std::monostate{};
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
        return std::monostate{};
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
        return std::monostate{};
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

        // Compile condition
        size_t exitJump = SIZE_MAX;
        auto* cond = node->getCondition();
        if (cond) {
            cond->accept(*this);
            exitJump = emitJump(bytecode::OpCode::JUMP_IF_FALSE);
        }

        // Jump over increment to body
        size_t bodyJump = emitJump(bytecode::OpCode::JUMP);

        // Increment position (continue target for for loops)
        size_t incrementStart = program.getCurrentOffset();
        auto* update = node->getUpdate();
        if (update) {
            update->accept(*this);
            emitWithLocation(bytecode::OpCode::POP, nullptr);  // Discard increment result
        }

        // Jump back to condition
        emitLoop(loopStart);

        // Patch body jump to here
        patchJump(bodyJump);

        // Enter loop with increment as continue target
        enterLoop(loopStart, incrementStart);

        // Compile body
        node->getBody()->accept(*this);

        // Jump to increment
        program.emit(bytecode::OpCode::JUMP, static_cast<uint32_t>(incrementStart));

        // Patch exit jump
        if (exitJump != SIZE_MAX) {
            patchJump(exitJump);
        }

        exitLoop();
        endScope();
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitBreakNode(ast::BreakNode* node)
    {
        if (!switchStack.empty()) {
            // Break in switch context
            size_t breakJump = emitJump(bytecode::OpCode::JUMP);
            switchStack.back().breakJumps.push_back(breakJump);
        } else if (!loopStack.empty()) {
            // Break in loop context
            size_t breakJump = emitJump(bytecode::OpCode::JUMP);
            currentLoop().breakJumps.push_back(breakJump);
        } else {
            throw errors::ParseException("Break outside of loop or switch");
        }
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitContinueNode(ast::ContinueNode* node)
    {
        if (loopStack.empty()) {
            throw errors::ParseException("Continue outside of loop");
        }

        size_t continueJump = emitJump(bytecode::OpCode::JUMP);
        currentLoop().continueJumps.push_back(continueJump);
        return std::monostate{};
    }

    // ==================== Placeholder Implementations ====================
    // These will be implemented in subsequent phases

    value::Value BytecodeCompiler::visitForEachNode(ast::ForEachNode* node)
    {
        // ForEach loop: for (Type element : collection) { body }
        // Compiles to:
        // 1. Evaluate collection
        // 2. Get iterator/length
        // 3. Loop: get next element, assign to variable, execute body

        std::string varName = node->getVariableName();
        value::ValueType varType = node->getVariableType();

        // Compile the collection expression
        node->getCollection()->accept(*this);

        // Duplicate collection on stack for iteration
        program.emit(bytecode::OpCode::DUP);

        // Get collection length/size
        // For arrays: use .length field
        // For collections: call .size() method or iterate directly
        program.emit(bytecode::OpCode::ARRAY_LENGTH);  // Will work for arrays

        // Initialize loop counter (stored in local variable)
        program.emit(bytecode::OpCode::PUSH_INT, static_cast<uint32_t>(program.getConstantPool().addInteger(0)));
        size_t counterIndex = program.getConstantPool().addString("__foreach_counter__");
        program.emit(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(0)); // Counter at slot 0

        // Loop start
        size_t loopStart = program.getCurrentOffset();

        // Load counter and length, compare
        program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(0)); // Load counter
        program.emit(bytecode::OpCode::SWAP); // Swap to get: counter, length
        program.emit(bytecode::OpCode::DUP);  // Duplicate length: counter, length, length
        program.emit(bytecode::OpCode::SWAP); // Swap: counter, length, length -> length, counter, length
        // Stack: length, counter, length

        // Compare counter < length
        size_t loopConditionOffset = program.getCurrentOffset();
        program.emit(bytecode::OpCode::LT); // counter < length

        // Jump to end if false
        size_t exitJump = emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        // Get current element: collection[counter]
        program.emit(bytecode::OpCode::DUP);  // Duplicate collection
        program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(0)); // Load counter
        program.emit(bytecode::OpCode::ARRAY_GET); // Get collection[counter]

        // Declare loop variable and assign current element
        size_t varNameIndex = program.getConstantPool().addString(varName);
        program.emit(bytecode::OpCode::DECLARE_VAR, static_cast<uint32_t>(varNameIndex));

        // Compile loop body
        auto* body = node->getBody();
        if (body) {
            // Track loop for break/continue
            LoopContext ctx;
            ctx.loopStart = loopStart;
            ctx.continueTarget = loopStart;  // Continue jumps back to loop condition
            loopStack.push_back(ctx);

            body->accept(*this);

            loopStack.pop_back();
        }

        // Increment counter
        program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(0)); // Load counter
        program.emit(bytecode::OpCode::INC); // Increment
        program.emit(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(0)); // Store counter

        // Jump back to loop start
        emitLoop(loopStart);

        // Patch exit jump
        patchJump(exitJump);

        // Clean up stack (pop collection and length)
        program.emit(bytecode::OpCode::POP);
        program.emit(bytecode::OpCode::POP);

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitSwitchNode(ast::SwitchNode* node)
    {
        // Enter switch context for break handling
        SwitchContext ctx;
        switchStack.push_back(ctx);

        // Compile the switch expression
        node->getExpression()->accept(*this);

        const auto& cases = node->getCases();
        std::vector<size_t> caseBodyStarts;  // Start positions of case bodies
        size_t defaultBodyStart = SIZE_MAX;   // Start position of default case body
        size_t defaultCaseIndex = SIZE_MAX;   // Index of default case

        // First pass: generate comparisons and jumps to case bodies
        for (size_t i = 0; i < cases.size(); ++i) {
            if (auto* defaultCase = dynamic_cast<ast::DefaultCaseNode*>(cases[i].get())) {
                defaultCaseIndex = i;
                caseBodyStarts.push_back(SIZE_MAX);  // Placeholder for default
            } else if (auto* regularCase = dynamic_cast<ast::CaseNode*>(cases[i].get())) {
                // Duplicate switch value for comparison
                emitWithLocation(bytecode::OpCode::DUP, regularCase);

                // Compile case value
                regularCase->getValue()->accept(*this);

                // Compare: switch_value == case_value
                emitWithLocation(bytecode::OpCode::EQ, regularCase);

                // Pop comparison result and jump to case body if true
                size_t jumpToCaseBody = emitJump(bytecode::OpCode::JUMP_IF_TRUE);
                caseBodyStarts.push_back(jumpToCaseBody);
            }
        }

        // If no case matched, jump to default or end
        size_t jumpToDefaultOrEnd = emitJump(bytecode::OpCode::JUMP);

        // Second pass: compile case bodies (with fallthrough support)
        for (size_t i = 0; i < cases.size(); ++i) {
            // Patch jump to this case body
            if (caseBodyStarts[i] != SIZE_MAX) {
                patchJump(caseBodyStarts[i]);
            }

            // Mark default case body start
            if (i == defaultCaseIndex) {
                defaultBodyStart = program.getCurrentOffset();
            }

            // Pop switch value only when entering the first matched case
            // (not during fallthrough)
            if (caseBodyStarts[i] != SIZE_MAX || i == defaultCaseIndex) {
                emitWithLocation(bytecode::OpCode::POP, cases[i].get());
            }

            // Compile case statements
            if (auto* defaultCase = dynamic_cast<ast::DefaultCaseNode*>(cases[i].get())) {
                for (const auto& stmt : defaultCase->getStatements()) {
                    stmt->accept(*this);
                }
            } else if (auto* regularCase = dynamic_cast<ast::CaseNode*>(cases[i].get())) {
                for (const auto& stmt : regularCase->getStatements()) {
                    stmt->accept(*this);
                }
            }
            // Note: No automatic jump to end - fallthrough is allowed!
        }

        // If execution falls through all cases without break, jump to end
        size_t implicitEndJump = emitJump(bytecode::OpCode::JUMP);

        // Patch jump to default or end
        patchJump(jumpToDefaultOrEnd);
        if (defaultBodyStart != SIZE_MAX) {
            // Jump to default case body
            program.emit(bytecode::OpCode::JUMP, static_cast<uint32_t>(defaultBodyStart));
        } else {
            // No default, just pop the switch value and continue
            emitWithLocation(bytecode::OpCode::POP, node);
        }

        // Patch all break jumps to end of switch (including implicit end jump)
        patchJump(implicitEndJump);
        for (size_t breakJump : switchStack.back().breakJumps) {
            patchJump(breakJump);
        }

        // Exit switch context
        switchStack.pop_back();

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitCaseNode(ast::CaseNode* node)
    {
        // Case nodes are handled by SwitchNode, not visited directly
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitDefaultCaseNode(ast::DefaultCaseNode* node)
    {
        // Default case nodes are handled by SwitchNode, not visited directly
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitFunctionNode(ast::FunctionNode* node)
    {
        std::string funcName = node->getName();

        // Emit JUMP to skip over function body during main execution
        size_t skipJump = emitJump(bytecode::OpCode::JUMP);

        // Function starts here
        size_t functionStart = program.getCurrentOffset();

        // Get parameters
        auto params = node->getParameters();
        std::vector<std::string> paramNames;
        for (const auto& param : params) {
            paramNames.push_back(param.first);
        }

        // Convert return type to string
        std::string returnTypeStr = evaluator::utils::ValueConverter::valueTypeToString(node->getReturnType());

        // Enter function frame for local variable tracking
        enterFunctionFrame(returnTypeStr);
        beginScope();  // Function body scope

        // Track parameters as locals
        for (const auto& param : params) {
            LocalVariable local;
            local.name = param.first;
            local.slot = nextLocalSlot++;
            local.scopeDepth = currentScopeDepth;
            locals.push_back(local);
        }

        // Compile function body
        auto* body = node->getBodyPtr();
        if (body) {
            body->accept(*this);
        }

        // Emit implicit return for void functions (if no explicit return)
        if (node->getReturnType() == value::ValueType::VOID) {
            program.emit(bytecode::OpCode::PUSH_NULL);
            program.emit(bytecode::OpCode::RETURN_VALUE);
        }

        // Calculate local count before exiting frame
        size_t localCount = getLocalCount();

        endScope();      // End function body scope
        exitFunctionFrame();

        size_t functionEnd = program.getCurrentOffset();

        // Patch skip jump to here (after function)
        patchJump(skipJump);

        // Register function metadata
        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = funcName;
        metadata.startOffset = functionStart;
        metadata.instructionCount = functionEnd - functionStart;
        metadata.localCount = localCount;
        metadata.parameterCount = params.size();
        metadata.parameterNames = paramNames;
        metadata.returnType = returnTypeStr;
        metadata.isNative = false;

        program.registerFunction(funcName, metadata);

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitFunctionCallNode(ast::FunctionCallNode* node)
    {
        // Compile all arguments (left to right)
        const auto& arguments = node->getArguments();
        for (const auto& arg : arguments) {
            arg->accept(*this);
        }

        // Get function name and add to constant pool
        std::string functionName = node->getFunctionName();
        size_t nameIndex = program.getConstantPool().addString(functionName);

        // Check if this is a static method call (ClassName::methodName)
        if (functionName.find("::") != std::string::npos) {
            // Static method call - use CALL_STATIC
            program.emit(bytecode::OpCode::CALL_STATIC,
                         static_cast<uint32_t>(nameIndex),
                         static_cast<uint32_t>(arguments.size()));
        } else {
            // Regular function call - use CALL
            program.emit(bytecode::OpCode::CALL,
                         static_cast<uint32_t>(nameIndex),
                         static_cast<uint32_t>(arguments.size()));
        }

        return std::monostate{};
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
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitNativeFunctionNode(ast::NativeFunctionNode* node)
    {
        throw std::runtime_error("NativeFunction compilation not yet implemented");
    }

    value::Value BytecodeCompiler::visitClassNode(ast::ClassNode* node)
    {
        std::string className = node->getClassName();

        // Set current class context for field access resolution
        auto previousClass = currentClassNode;
        currentClassNode = node;

        // Register class metadata (for runtime type checking and instanceof)
        size_t classNameIndex = program.getConstantPool().addString(className);

        // Handle generics - store generic parameter names
        if (node->isGeneric()) {
            const auto& genericParams = node->getGenericParameters();
            for (const auto& param : genericParams) {
                size_t paramNameIndex = program.getConstantPool().addString(param.name);
                // Generic parameters are stored in constant pool for type resolution
            }
        }

        // Handle inheritance - store parent class name if present
        if (node->hasParentClass()) {
            std::string parentClassName = node->getParentClassName();
            size_t parentNameIndex = program.getConstantPool().addString(parentClassName);
            // The parent class relationship is handled at runtime by the environment
            // We just need to ensure the metadata is available
        }

        // Handle interfaces - store interface names
        const auto& interfaces = node->getImplementedInterfaces();
        for (const auto& interfaceName : interfaces) {
            size_t interfaceNameIndex = program.getConstantPool().addString(interfaceName);
            // Interfaces are also handled at runtime
        }

        // Classes are registered at compile-time in the environment
        // The actual class instantiation happens with NEW_OBJECT instruction

        // Compile static fields initialization
        for (const auto& field : node->getFields()) {
            if (auto* fieldNode = dynamic_cast<ast::FieldNode*>(field.get())) {
                if (fieldNode->getIsStatic()) {
                    field->accept(*this);
                }
            }
        }

        // Compile static methods (they're like standalone functions)
        for (const auto& method : node->getMethods()) {
            if (auto* methodNode = dynamic_cast<ast::MethodNode*>(method.get())) {
                if (methodNode->getIsStatic()) {
                    method->accept(*this);
                }
            }
        }

        // Compile constructors
        for (const auto& constructor : node->getConstructors()) {
            constructor->accept(*this);
        }

        // Compile instance methods
        for (const auto& method : node->getMethods()) {
            if (auto* methodNode = dynamic_cast<ast::MethodNode*>(method.get())) {
                if (!methodNode->getIsStatic()) {
                    method->accept(*this);
                }
            }
        }

        // Restore previous class context
        currentClassNode = previousClass;

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitMethodNode(ast::MethodNode* node)
    {
        std::string methodName = node->getName();
        bool isStatic = node->getIsStatic();

        // Set instance method context
        bool wasInInstanceMethod = inInstanceMethod;
        inInstanceMethod = !isStatic;

        // Handle generic methods - store generic type parameter names
        if (node->isGeneric()) {
            const auto& genericParams = node->getGenericTypeParameters();
            for (const auto& param : genericParams) {
                size_t paramNameIndex = program.getConstantPool().addString(param.name);
                // Generic type parameters are available at runtime for type resolution
            }
        }

        // Emit JUMP to skip over method body during main execution
        size_t skipJump = emitJump(bytecode::OpCode::JUMP);

        // Method starts here
        size_t methodStart = program.getCurrentOffset();

        // Get parameters
        auto params = node->getParameters();
        std::vector<std::string> paramNames;
        for (const auto& param : params) {
            paramNames.push_back(param.first);
        }

        // For instance methods, 'this' is implicitly the first parameter
        if (!isStatic) {
            paramNames.insert(paramNames.begin(), "this");
        }

        // Convert return type to string
        value::ValueType returnType = node->getReturnType();
        std::string returnTypeStr = evaluator::utils::ValueConverter::valueTypeToString(returnType);

        // Enter function frame for local variable tracking
        enterFunctionFrame(returnTypeStr);
        beginScope();  // Method body scope

        // Track parameters as locals (including 'this' for instance methods)
        for (const auto& paramName : paramNames) {
            LocalVariable local;
            local.name = paramName;
            local.slot = nextLocalSlot++;
            local.scopeDepth = currentScopeDepth;
            locals.push_back(local);
        }

        // Compile method body
        auto* body = node->getBodyPtr();
        if (body) {
            body->accept(*this);
        }

        // Restore instance method context
        inInstanceMethod = wasInInstanceMethod;

        // Emit implicit return for void methods (if no explicit return)
        if (returnType == value::ValueType::VOID) {
            program.emit(bytecode::OpCode::PUSH_NULL);
            program.emit(bytecode::OpCode::RETURN_VALUE);
        }

        // Calculate local count before exiting frame
        size_t localCount = getLocalCount();

        endScope();      // End method body scope
        exitFunctionFrame();

        size_t methodEnd = program.getCurrentOffset();

        // Patch skip jump to here (after method)
        patchJump(skipJump);

        // Build qualified method name for registry
        std::string qualifiedMethodName = methodName;
        if (currentClassNode) {
            qualifiedMethodName = currentClassNode->getClassName() + "::" + methodName;
        }

        // Register method metadata
        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = qualifiedMethodName;
        metadata.startOffset = methodStart;
        metadata.instructionCount = methodEnd - methodStart;
        metadata.localCount = localCount;
        metadata.parameterCount = paramNames.size();
        metadata.parameterNames = paramNames;
        metadata.returnType = returnTypeStr;
        metadata.isStatic = isStatic;
        metadata.isNative = false;

        program.registerFunction(qualifiedMethodName, metadata);

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitConstructorNode(ast::ConstructorNode* node)
    {
        // Constructors are compiled similar to methods, but with special handling
        // They implicitly return 'this' after initialization

        // Set instance method context (constructors are like instance methods)
        bool wasInInstanceMethod = inInstanceMethod;
        inInstanceMethod = true;

        // Emit JUMP to skip over constructor body during main execution
        size_t skipJump = emitJump(bytecode::OpCode::JUMP);

        // Constructor starts here
        size_t constructorStart = program.getCurrentOffset();

        // Get parameters
        auto params = node->getParameters();
        std::vector<std::string> paramNames;
        paramNames.push_back("this");  // 'this' is always the first parameter for constructors
        for (const auto& param : params) {
            paramNames.push_back(param.first);
        }

        // Constructor returns an object instance
        std::string returnTypeStr = evaluator::utils::ValueConverter::valueTypeToString(value::ValueType::OBJECT);

        // Enter function frame for local variable tracking
        enterFunctionFrame(returnTypeStr);
        beginScope();  // Constructor body scope

        // Track parameters as locals (including 'this')
        for (const auto& paramName : paramNames) {
            LocalVariable local;
            local.name = paramName;
            local.slot = nextLocalSlot++;
            local.scopeDepth = currentScopeDepth;
            locals.push_back(local);
        }

        // Check if parent class requires super() call
        if (currentClassNode && currentClassNode->hasParentClass()) {
            std::string parentClassName = currentClassNode->getParentClassName();
            // Check if parent has any constructors defined
            auto parentClassDef = environment->getClassRegistry()->findClass(parentClassName);
            if (parentClassDef && !parentClassDef->getConstructors().empty()) {
                // Parent has constructors - super() call is required
                if (!node->hasSuperInitializer()) {
                    throw errors::RuntimeException(
                        "Constructor must call super() when parent class '" + parentClassName + "' has constructors"
                    );
                }
            }
        }

        // Handle super constructor call if present
        if (node->hasSuperInitializer()) {
            auto* superInit = node->getSuperInitializer();
            if (superInit) {
                superInit->accept(*this);
            }
        }

        // Compile constructor body
        auto* body = node->getBodyPtr();
        if (body) {
            body->accept(*this);
        }

        // Restore instance method context
        inInstanceMethod = wasInInstanceMethod;

        // Constructors implicitly return 'this'
        // Load 'this' parameter (first local at slot 0)
        program.emit(bytecode::OpCode::LOAD_LOCAL, 0);
        program.emit(bytecode::OpCode::RETURN_VALUE);

        // Calculate local count before exiting frame
        size_t localCount = getLocalCount();

        endScope();      // End constructor body scope
        exitFunctionFrame();

        size_t constructorEnd = program.getCurrentOffset();

        // Patch skip jump to here (after constructor)
        patchJump(skipJump);

        // Register constructor metadata (with special name "<init>")
        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = "<init>";
        metadata.startOffset = constructorStart;
        metadata.instructionCount = constructorEnd - constructorStart;
        metadata.localCount = localCount;
        metadata.parameterCount = paramNames.size();
        metadata.parameterNames = paramNames;
        metadata.returnType = returnTypeStr;
        metadata.isStatic = false;
        metadata.isNative = false;

        // Register constructor with parameter count to support overloading
        // Format: ClassName::<init>/<paramCount> (e.g., "MyClass::<init>/0", "MyClass::<init>/2")
        std::string className = currentClassNode ? currentClassNode->getClassName() : "";
        std::string constructorName = className + "::<init>/" + std::to_string(params.size());
        program.registerFunction(constructorName, metadata);

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitFieldNode(ast::FieldNode* node)
    {
        std::string fieldName = node->getName();
        bool isStatic = node->getIsStatic();

        // Only compile static fields here (instance fields are initialized in constructor)
        if (isStatic) {
            // Get initial value or null
            auto* initValue = node->getInitialValue();
            if (initValue) {
                initValue->accept(*this);
            } else {
                emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
            }

            // Store in static field with fully qualified name: ClassName::fieldName
            std::string qualifiedName = fieldName;
            if (currentClassNode) {
                qualifiedName = currentClassNode->getClassName() + "::" + fieldName;
            }
            size_t nameIndex = program.getConstantPool().addString(qualifiedName);

            // Use SET_STATIC_INIT which only sets if variable doesn't exist
            // This prevents overwriting values that have been modified
            emitWithLocation(bytecode::OpCode::SET_STATIC, static_cast<uint32_t>(nameIndex), node);
        }

        // Instance fields are not compiled here - they're handled during object creation

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitNewNode(ast::NewNode* node)
    {
        std::string fullClassName = node->getClassName();

        // Parse generic type arguments from className (e.g., "Box<Int>" -> "Box" and ["Int"])
        // The className may contain generic type arguments like "Box<Int>" or "Map<String,Int>"
        std::string baseClassName = fullClassName;
        std::vector<std::string> typeArguments;

        size_t genericStart = fullClassName.find('<');
        if (genericStart != std::string::npos) {
            baseClassName = fullClassName.substr(0, genericStart);

            // Extract type arguments from within <>
            size_t genericEnd = fullClassName.rfind('>');
            if (genericEnd != std::string::npos && genericEnd > genericStart) {
                std::string typeArgsStr = fullClassName.substr(genericStart + 1, genericEnd - genericStart - 1);

                // Store full generic class name (Box<Int>) for proper type identification
                // This is used at runtime to create the correct type bindings
            }
        }

        // Push constructor arguments onto stack (left to right)
        const auto& arguments = node->getArguments();
        for (const auto& arg : arguments) {
            arg->accept(*this);
        }

        // Store the FULL class name including generics (e.g., "Box<Int>")
        // The VM will parse this to create proper type bindings
        size_t classNameIndex = program.getConstantPool().addString(fullClassName);

        // Emit NEW_OBJECT instruction with full class name and argument count
        program.emit(bytecode::OpCode::NEW_OBJECT,
                     static_cast<uint32_t>(classNameIndex),
                     static_cast<uint32_t>(arguments.size()));

        // NEW_OBJECT will:
        // 1. Parse generic type arguments from class name
        // 2. Pop N arguments from stack
        // 3. Create new object instance with type bindings
        // 4. Call constructor with arguments
        // 5. Push resulting object onto stack

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitMemberAccessNode(ast::MemberAccessNode* node)
    {
        std::string memberName = node->getMemberName();
        bool isStaticAccess = node->getIsStaticAccess();

        if (isStaticAccess) {
            // Static field access: ClassName::fieldName
            // The memberName already includes the full qualified name (e.g., "Book::TEST")
            // from the parser, so we use it directly
            size_t fieldNameIndex = program.getConstantPool().addString(memberName);
            emitWithLocation(bytecode::OpCode::GET_STATIC, static_cast<uint32_t>(fieldNameIndex), node);
        } else {
            // Instance field access: object.fieldName
            // First, compile the object expression
            node->getObject()->accept(*this);

            // Then emit GET_FIELD instruction
            size_t fieldNameIndex = program.getConstantPool().addString(memberName);
            emitWithLocation(bytecode::OpCode::GET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);
        }

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitMemberAssignmentNode(ast::MemberAssignmentNode* node)
    {
        std::string memberName = node->getMemberName();

        // Compile the object expression
        node->getObject()->accept(*this);

        // Compile the value to assign
        node->getValue()->accept(*this);

        // Emit SET_FIELD instruction (object and value are on stack)
        size_t fieldNameIndex = program.getConstantPool().addString(memberName);
        emitWithLocation(bytecode::OpCode::SET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitMethodCallNode(ast::MethodCallNode* node)
    {
        std::string methodName = node->getMethodName();
        bool isStaticCall = node->getIsStaticCall();
        const auto& arguments = node->getArguments();

        if (isStaticCall) {
            // Static method call: ClassName::methodName(args)
            // Push all arguments onto stack
            for (const auto& arg : arguments) {
                arg->accept(*this);
            }

            // Emit CALL_STATIC instruction
            size_t methodNameIndex = program.getConstantPool().addString(methodName);
            program.emit(bytecode::OpCode::CALL_STATIC,
                         static_cast<uint32_t>(methodNameIndex),
                         static_cast<uint32_t>(arguments.size()));
        } else {
            // Instance method call: object.methodName(args)
            // First, compile the object expression
            node->getObject()->accept(*this);

            // Push all arguments onto stack
            for (const auto& arg : arguments) {
                arg->accept(*this);
            }

            // Emit CALL_METHOD instruction
            size_t methodNameIndex = program.getConstantPool().addString(methodName);
            program.emit(bytecode::OpCode::CALL_METHOD,
                         static_cast<uint32_t>(methodNameIndex),
                         static_cast<uint32_t>(arguments.size()));
        }

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitSuperConstructorCallNode(ast::SuperConstructorCallNode* node)
    {
        // Push 'this' onto stack (for super constructor to initialize)
        size_t thisNameIndex = program.getConstantPool().addString("this");
        program.emit(bytecode::OpCode::LOAD_VAR, static_cast<uint32_t>(thisNameIndex));

        // Push arguments onto stack
        const auto& arguments = node->getArguments();
        for (const auto& arg : arguments) {
            arg->accept(*this);
        }

        // Emit SUPER_CONSTRUCTOR instruction
        // This will call the parent class constructor
        program.emit(bytecode::OpCode::SUPER_CONSTRUCTOR, static_cast<uint32_t>(arguments.size()));

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitSuperMethodCallNode(ast::SuperMethodCallNode* node)
    {
        std::string methodName = node->getMethodName();

        // Push 'this' onto stack
        size_t thisNameIndex = program.getConstantPool().addString("this");
        program.emit(bytecode::OpCode::LOAD_VAR, static_cast<uint32_t>(thisNameIndex));

        // Push arguments onto stack
        const auto& arguments = node->getArguments();
        for (const auto& arg : arguments) {
            arg->accept(*this);
        }

        // Emit SUPER_INVOKE instruction
        size_t methodNameIndex = program.getConstantPool().addString(methodName);
        program.emit(bytecode::OpCode::SUPER_INVOKE,
                     static_cast<uint32_t>(methodNameIndex),
                     static_cast<uint32_t>(arguments.size()));

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitInterfaceNode(ast::InterfaceNode* node)
    {
        std::string interfaceName = node->getName();

        // Register interface metadata in constant pool
        size_t interfaceNameIndex = program.getConstantPool().addString(interfaceName);

        // Store extended interfaces
        const auto& extendedInterfaces = node->getExtendedInterfaces();
        for (const auto& parentInterface : extendedInterfaces) {
            size_t parentNameIndex = program.getConstantPool().addString(parentInterface);
        }

        // Interfaces are purely compile-time contracts in bytecode
        // Method signatures are validated at compile time
        // The runtime will use the interface registry from the environment
        // No actual bytecode is generated for interfaces themselves

        // Compile method signatures (for validation purposes)
        const auto& methods = node->getMethods();
        for (const auto& method : methods) {
            // Method signatures are just stored as metadata, no bytecode emitted
            // The actual implementation will be in classes that implement this interface
        }

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitArrayCreationNode(ast::ArrayCreationNode* node)
    {
        const auto& sizeExpressions = node->getSizeExpressions();
        size_t dimensionCount = node->getDimensionCount();

        if (dimensionCount == 1) {
            // Single-dimensional array: new Type[size]
            // Compile size expression and push onto stack
            sizeExpressions[0]->accept(*this);

            // Get element type info
            const auto& typeInfo = node->getElementTypeInfo();
            size_t typeNameIndex = program.getConstantPool().addString(typeInfo.toString());

            // Emit NEW_ARRAY with element type
            program.emit(bytecode::OpCode::NEW_ARRAY, static_cast<uint32_t>(typeNameIndex));
        }
        else {
            // Multi-dimensional array: new Type[size1][size2]...
            // Push all dimension sizes onto stack (in order)
            for (const auto& sizeExpr : sizeExpressions) {
                sizeExpr->accept(*this);
            }

            // Get element type info
            const auto& typeInfo = node->getElementTypeInfo();
            size_t typeNameIndex = program.getConstantPool().addString(typeInfo.toString());

            // Emit NEW_ARRAY_MULTI with element type and dimension count
            program.emit(bytecode::OpCode::NEW_ARRAY_MULTI,
                        static_cast<uint32_t>(typeNameIndex),
                        static_cast<uint32_t>(dimensionCount));
        }

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitArrayLiteralNode(ast::ArrayLiteralNode* node)
    {
        const auto& elements = node->getElements();
        size_t elementCount = elements.size();

        // Push array size onto stack
        program.emit(bytecode::OpCode::PUSH_INT, static_cast<uint32_t>(program.getConstantPool().addInteger(static_cast<int>(elementCount))));

        // Create array with generic "Object" type (type will be inferred from elements)
        size_t typeNameIndex = program.getConstantPool().addString("Object");
        program.emit(bytecode::OpCode::NEW_ARRAY, static_cast<uint32_t>(typeNameIndex));

        // Array is now on stack. For each element:
        // 1. Duplicate array reference
        // 2. Push index
        // 3. Push element value
        // 4. Call ARRAY_SET
        for (size_t i = 0; i < elementCount; ++i) {
            program.emit(bytecode::OpCode::DUP);  // Duplicate array reference

            // Push index
            program.emit(bytecode::OpCode::PUSH_INT, static_cast<uint32_t>(program.getConstantPool().addInteger(static_cast<int>(i))));

            // Compile and push element value
            elements[i]->accept(*this);

            // Set array element
            program.emit(bytecode::OpCode::ARRAY_SET);
        }

        // Array reference is still on stack
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitIndexAccessNode(ast::IndexAccessNode* node)
    {
        // Compile array/collection expression
        node->getCollection()->accept(*this);

        // Compile index expression
        node->getIndex()->accept(*this);

        // Emit ARRAY_GET to retrieve element
        // Stack before: [array, index]
        // Stack after: [element]
        program.emit(bytecode::OpCode::ARRAY_GET);

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitIndexAssignmentNode(ast::IndexAssignmentNode* node)
    {
        // Compile array/collection expression
        node->getObject()->accept(*this);

        // Compile index expression
        node->getIndex()->accept(*this);

        // Compile value expression
        node->getValue()->accept(*this);

        // Emit ARRAY_SET to store element
        // Stack before: [array, index, value]
        // Stack after: [] (all consumed)
        program.emit(bytecode::OpCode::ARRAY_SET);

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitLambdaNode(ast::LambdaNode* node)
    {
        // Generate unique lambda function name
        static size_t lambdaCounter = 0;
        std::string lambdaFuncName = "__lambda_" + std::to_string(lambdaCounter++);
        size_t lambdaNameIndex = program.getConstantPool().addString(lambdaFuncName);

        // Store current position to jump over lambda body
        program.emit(bytecode::OpCode::JUMP, 0); // Placeholder
        size_t skipJump = program.getCurrentOffset() - 1;

        // Lambda function starts here
        size_t lambdaStart = program.getCurrentOffset();

        // Get parameters
        const auto& params = node->getParameters();

        // Capture variables from outer scope for closure support
        // Save current closure captures
        std::vector<ClosureVariable> previousCaptures = closureCaptures;
        closureCaptures.clear();

        // Scan lambda body for variable references and capture from outer scopes
        // For now, we capture all locals from parent scopes
        for (const auto& local : locals) {
            ClosureVariable capture;
            capture.name = local.name;
            capture.slot = local.slot;
            capture.isFromParent = true;
            closureCaptures.push_back(capture);
        }

        // Emit captured variables onto stack (they will be part of lambda closure)
        for (const auto& capture : closureCaptures) {
            size_t nameIndex = program.getConstantPool().addString(capture.name);
            program.emit(bytecode::OpCode::LOAD_VAR, static_cast<uint32_t>(nameIndex));
        }

        // Enter function frame for lambda
        enterFunctionFrame("auto");
        beginScope();

        // Track lambda parameters as locals
        for (const auto& param : params) {
            LocalVariable local;
            local.name = param.name;
            local.slot = nextLocalSlot++;
            local.scopeDepth = currentScopeDepth;
            locals.push_back(local);
        }

        // Add captured variables as locals (so they can be referenced in lambda body)
        for (const auto& capture : closureCaptures) {
            LocalVariable local;
            local.name = capture.name;
            local.slot = nextLocalSlot++;
            local.scopeDepth = currentScopeDepth;
            locals.push_back(local);
        }

        // Compile lambda body
        auto* body = node->getBody();
        if (node->isExpressionLambda()) {
            // Expression lambda: () -> expr
            // Compile expression and return its value
            body->accept(*this);
            program.emit(bytecode::OpCode::RETURN_VALUE);
        }
        else {
            // Block lambda: () -> { ... }
            // Compile block
            body->accept(*this);

            // Implicit return null if no explicit return
            program.emit(bytecode::OpCode::PUSH_NULL);
            program.emit(bytecode::OpCode::RETURN_VALUE);
        }

        endScope();
        exitFunctionFrame();

        // Lambda function ends here
        size_t lambdaEnd = program.getCurrentOffset();

        // Patch the skip jump to here
        program.patchJump(skipJump, static_cast<uint32_t>(lambdaEnd));

        // Now emit instruction to create lambda value with captured environment
        // Store lambda function start address, parameter count, and capture count
        program.emit(bytecode::OpCode::LAMBDA,
                    std::vector<uint32_t>{
                        static_cast<uint32_t>(lambdaStart),
                        static_cast<uint32_t>(params.size()),
                        static_cast<uint32_t>(closureCaptures.size())
                    });

        // Restore previous closure captures
        closureCaptures = previousCaptures;

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitCastExpression(ast::CastExpression* node)
    {
        // Compile the expression to be cast
        node->getExpression()->accept(*this);

        // Get target type information
        const auto* targetType = node->getTargetType();
        std::string targetTypeName = targetType->toString();

        // Store target type name in constant pool
        size_t typeNameIndex = program.getConstantPool().addString(targetTypeName);

        // Emit CAST instruction with target type
        // Stack before: [value]
        // Stack after: [casted_value]
        program.emit(bytecode::OpCode::CAST, static_cast<uint32_t>(typeNameIndex));

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitInstanceOfExpression(ast::InstanceOfExpression* node)
    {
        // Compile the expression to check
        node->getExpression()->accept(*this);

        // Get target type information
        const auto* targetType = node->getTargetType();
        std::string targetTypeName = targetType->toString();

        // Store target type name in constant pool
        size_t typeNameIndex = program.getConstantPool().addString(targetTypeName);

        // Emit INSTANCEOF instruction with target type
        // Stack before: [value]
        // Stack after: [bool_result]
        program.emit(bytecode::OpCode::INSTANCEOF, static_cast<uint32_t>(typeNameIndex));

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitImportNode(ast::ImportNode* node)
    {
        // For bytecode compilation, imports are handled at compile time
        // The imported AST has already been processed and symbols registered in the environment
        // We need to compile any imported declarations that aren't already compiled

        // Get the imported AST
        auto* importedAST = node->getImportedAST();

        if (!importedAST) {
            // Import not resolved - this is an error
            throw std::runtime_error("Import not resolved: " + node->getFilePath());
        }

        // Compile the imported AST
        // This will add any functions, classes, etc. to our bytecode program
        importedAST->accept(*this);

        // Imports don't produce a runtime value
        return std::monostate{};
    }

} // namespace vm::compiler
