#include "BytecodeCompiler.hpp"
#include "../../errors/ParseException.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../evaluator/utils/ValueConverter.hpp"
#include <typeinfo>
#include <unordered_set>
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

        // Second, establish parent-child relationships between classes
        linkParentClasses(root);

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
            // Use base class name for registration (VM looks up by base name)
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
            if (!classNode->getConstructors().empty()) {
                for (const auto& constructor : classNode->getConstructors()) {
                    if (auto* ctorNode = dynamic_cast<ast::nodes::classes::ConstructorNode*>(constructor.get())) {
                        auto ctorDef = std::make_shared<runtimeTypes::klass::ConstructorDefinition>(
                            ctorNode->getParametersWithTypes(),
                            ctorNode->getBody()
                        );
                        classDef->addConstructor(ctorDef);
                    }
                }
            } else {
                // No explicit constructors - register a default constructor
                // This is needed so ClassDefinition knows a constructor exists (even though it's in bytecode)
                std::vector<std::pair<std::string, value::ParameterType>> emptyParams;
                auto defaultCtor = std::make_shared<runtimeTypes::klass::ConstructorDefinition>(
                    emptyParams,
                    nullptr  // No AST body - bytecode will handle it
                );
                classDef->addConstructor(defaultCtor);
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
        else if (auto importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(node))
        {
            // Process classes from imported AST
            if (importNode->isResolved() && importNode->getImportedAST())
            {
                registerClassesForBytecode(importNode->getImportedAST());
            }
        }
    }

    void BytecodeCompiler::linkParentClasses(ast::ASTNode* node)
    {
        if (!node) return;

        // Check if this node is a ClassNode with a parent
        if (auto classNode = dynamic_cast<ast::ClassNode*>(node))
        {
            if (classNode->hasParentClass()) {
                std::string className = classNode->getClassName();
                std::string parentClassName = classNode->getParentClassName();

                auto classRegistry = environment->getClassRegistry();
                if (!classRegistry) {
                    throw std::runtime_error("Class registry not available");
                }

                // Strip generic type parameters from parent class name: "Container<T>" -> "Container"
                size_t genericStart = parentClassName.find('<');
                std::string baseParentClassName = (genericStart != std::string::npos)
                    ? parentClassName.substr(0, genericStart)
                    : parentClassName;

                // Get both class definitions
                auto classDef = classRegistry->findClass(className);
                auto parentDef = classRegistry->findClass(baseParentClassName);

                if (classDef && parentDef) {
                    // Establish the parent-child link
                    classDef->setParentClass(parentDef);
                }
            }
            return; // No need to traverse children of ClassNode
        }

        // Recursively process child nodes
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                linkParentClasses(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                linkParentClasses(statement.get());
            }
        }
        else if (auto importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(node))
        {
            // Process classes from imported AST
            if (importNode->isResolved() && importNode->getImportedAST())
            {
                linkParentClasses(importNode->getImportedAST());
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
        frame.localStartSlot = nextLocalSlot;  // Remember where this function's locals start
        frame.scopeDepthStart = currentScopeDepth;
        frame.returnType = returnType;
        functionFrameStack.push_back(frame);
        // NOTE: nextLocalSlot continues from where it was - NOT reset to 0
        // The resolveLocal function will return relative slots by subtracting localStartSlot
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

            // Restore nextLocalSlot and scope depth to values before entering this function
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

        // Check if this is a local variable FIRST (parameters and local vars take precedence over fields)
        size_t localSlot = resolveLocal(name);
        if (localSlot != SIZE_MAX) {
            // This is a local variable - use LOAD_LOCAL with slot index
            emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(localSlot), node);
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
        value::ValueType valueType = value::ValueType::VOID;
        try {
            valueType = node->getType();
        } catch (...) {
            // If getType() fails (e.g., for generic types like Node<T>), treat as OBJECT
            valueType = value::ValueType::OBJECT;
        }

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
        auto* value = node->getValue();
        if (value) {
            value->accept(*this);
        } else {
            // Declaration without initializer - push null
            program.emit(bytecode::OpCode::PUSH_NULL);
        }

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
            // Check if operand is a field access (e.g., this.count++)
            else if (auto* memberNode = dynamic_cast<ast::MemberAccessNode*>(node->getOperand())) {
                std::string fieldName = memberNode->getMemberName();
                size_t fieldNameIndex = program.getConstantPool().addString(fieldName);

                bool isPostfix = (node->getPosition() == ast::nodes::expressions::UnaryPosition::POSTFIX);
                bytecode::OpCode opcode = (op == token::TokenType::INCREMENT)
                    ? bytecode::OpCode::INC
                    : bytecode::OpCode::DEC;

                // Simpler strategy: Load object twice
                // 1. Load obj, get field value
                // 2. Save original if postfix, increment
                // 3. Load obj again, set field to incremented
                // 4. Return appropriate value (original for postfix, incremented for prefix)

                // Step 1: Get current value
                memberNode->getObject()->accept(*this);  // [obj]
                emitWithLocation(bytecode::OpCode::DUP, node);  // [obj, obj]
                emitWithLocation(bytecode::OpCode::GET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);  // [obj, val]

                // Step 2: Calculate new value
                if (isPostfix) {
                    emitWithLocation(bytecode::OpCode::DUP, node);  // [obj, val, val] - save original
                }
                emitWithLocation(opcode, node);  // [obj, val, newval] (postfix) or [obj, newval] (prefix)

                // Step 3: Prepare for SET_FIELD
                // SET_FIELD needs [obj, newval] with newval on top
                if (isPostfix) {
                    // We have [obj, oldval, newval]
                    // We need [oldval, obj, newval] so that SET_FIELD consumes [obj, newval] and leaves [oldval]

                    // Pop newval temporarily, swap obj and oldval, push newval back
                    // Actually that won't work with the stack model...

                    // Different approach: Load obj again
                    // Currently [obj, oldval, newval]
                    // We want to: store newval to field, return oldval
                    // Problem: obj is buried in stack

                    // Solution: compile object again
                    memberNode->getObject()->accept(*this);  // [obj, oldval, newval, obj]
                    emitWithLocation(bytecode::OpCode::SWAP, node);  // [obj, oldval, obj, newval]
                    emitWithLocation(bytecode::OpCode::SET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);  // [obj, oldval, newval] (SET_FIELD pushes value)
                    emitWithLocation(bytecode::OpCode::POP, node);  // [obj, oldval] - discard pushed value
                    emitWithLocation(bytecode::OpCode::SWAP, node);  // [oldval, obj]
                    emitWithLocation(bytecode::OpCode::POP, node);  // [oldval] - discard obj
                } else {
                    // We have [obj, newval]
                    // Dup the new value for return, then set field
                    emitWithLocation(bytecode::OpCode::DUP, node);  // [obj, newval, newval]
                    emitWithLocation(bytecode::OpCode::SWAP, node);  // [obj, newval, newval] -> [newval, obj, newval]
                    // Wait that's wrong... let me reconsider

                    // [obj, newval] -> want to return newval and store to field
                    // SET_FIELD: pops value, pops obj, stores, pushes value
                    emitWithLocation(bytecode::OpCode::SET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);  // pushes newval back
                    // Stack now has newval, which is what we want for prefix
                }

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
        // Strategy: Desugar to regular for loop
        // for (Type element : array) { body }
        // becomes:
        // Type[] __temp = array;
        // for (int __i = 0; __i < __temp.length; __i++) {
        //     Type element = __temp[__i];
        //     body;
        // }

        std::string varName = node->getVariableName();
        value::ValueType varType = node->getVariableType();

        beginScope();

        // Evaluate and store the collection in a local variable
        node->getCollection()->accept(*this);

        // Store collection in local variable (at current slot)
        LocalVariable arrayLocal;
        arrayLocal.name = "__foreach_array__";
        arrayLocal.slot = nextLocalSlot++;
        arrayLocal.scopeDepth = currentScopeDepth;
        locals.push_back(arrayLocal);
        // Collection is already on stack, it stays there as the local

        // Get array length and store it in a local variable
        program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(arrayLocal.slot));
        program.emit(bytecode::OpCode::ARRAY_LENGTH);

        LocalVariable lengthLocal;
        lengthLocal.name = "__foreach_length__";
        lengthLocal.slot = nextLocalSlot++;
        lengthLocal.scopeDepth = currentScopeDepth;
        locals.push_back(lengthLocal);
        // Length is on stack, it stays there as the local

        // Initialize counter to 0
        program.emit(bytecode::OpCode::PUSH_INT, static_cast<uint32_t>(program.getConstantPool().addInteger(0)));

        LocalVariable counterLocal;
        counterLocal.name = "__foreach_counter__";
        counterLocal.slot = nextLocalSlot++;
        counterLocal.scopeDepth = currentScopeDepth;
        locals.push_back(counterLocal);
        // Counter is on stack, it stays there as the local

        // Loop start - check condition: counter < length
        size_t loopStart = program.getCurrentOffset();

        program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(counterLocal.slot));
        program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(lengthLocal.slot));
        program.emit(bytecode::OpCode::LT);

        size_t exitJump = emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        // Get current element: array[counter]
        program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(arrayLocal.slot));
        program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(counterLocal.slot));
        program.emit(bytecode::OpCode::ARRAY_GET);

        // Store in loop variable as a local
        LocalVariable loopVar;
        loopVar.name = varName;
        loopVar.slot = nextLocalSlot++;
        loopVar.scopeDepth = currentScopeDepth;
        locals.push_back(loopVar);
        // Element is on stack, it stays there as the local

        // Compile loop body
        auto* body = node->getBody();
        if (body) {
            // Track loop for break/continue
            size_t continueTarget = program.getCurrentOffset();
            LoopContext ctx;
            ctx.loopStart = loopStart;
            ctx.continueTarget = continueTarget;
            loopStack.push_back(ctx);

            body->accept(*this);

            loopStack.pop_back();
        }

        // Pop loop variable from stack (end of iteration)
        program.emit(bytecode::OpCode::POP);

        // Remove loop variable from locals
        locals.pop_back();
        nextLocalSlot--;

        // Increment counter: counter++
        program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(counterLocal.slot));
        program.emit(bytecode::OpCode::PUSH_INT, static_cast<uint32_t>(program.getConstantPool().addInteger(1)));
        program.emit(bytecode::OpCode::ADD);
        program.emit(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(counterLocal.slot));
        program.emit(bytecode::OpCode::POP); // Pop the result of assignment

        // Jump back to loop start
        emitLoop(loopStart);

        // Patch exit jump
        patchJump(exitJump);

        // Clean up: pop counter, length, and array from stack
        program.emit(bytecode::OpCode::POP); // counter
        program.emit(bytecode::OpCode::POP); // length
        program.emit(bytecode::OpCode::POP); // array

        endScope();

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

        // Check if this is a static method call (ClassName::methodName)
        if (functionName.find("::") != std::string::npos) {
            // Replace "this::" with actual class name if inside a class
            if (functionName.substr(0, 6) == "this::" && currentClassNode) {
                std::string methodName = functionName.substr(6); // Remove "this::"
                functionName = currentClassNode->getClassName() + "::" + methodName;
            }

            size_t nameIndex = program.getConstantPool().addString(functionName);
            // Static method call - use CALL_STATIC
            program.emit(bytecode::OpCode::CALL_STATIC,
                         static_cast<uint32_t>(nameIndex),
                         static_cast<uint32_t>(arguments.size()));
        } else {
            size_t nameIndex = program.getConstantPool().addString(functionName);
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
        // Use full class name with generic parameters for generic classes
        std::string className = node->isGeneric() ?
            node->getFullClassName() : node->getClassName();


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
        if (!node->getConstructors().empty()) {
            for (const auto& constructor : node->getConstructors()) {
                constructor->accept(*this);
            }
        } else {
            // No explicit constructor - generate a default one that initializes fields
            // Emit JUMP to skip over default constructor during main execution
            size_t skipJump = emitJump(bytecode::OpCode::JUMP);
            size_t constructorStart = program.getCurrentOffset();

            // Default constructor has only 'this' as parameter
            std::vector<std::string> paramNames = {"this"};

            // Enter function frame
            enterFunctionFrame("object");
            beginScope();

            // Track 'this' as local
            LocalVariable thisLocal;
            thisLocal.name = "this";
            thisLocal.slot = nextLocalSlot++;
            thisLocal.scopeDepth = currentScopeDepth;
            locals.push_back(thisLocal);

            // Initialize instance fields with their default values
            auto& fields = node->getFields();
            for (const auto& fieldPtr : fields) {
                if (auto* fieldNode = dynamic_cast<ast::FieldNode*>(fieldPtr.get())) {
                    // Only initialize instance fields (not static)
                    if (!fieldNode->getIsStatic() && fieldNode->getInitialValue()) {
                        // Load 'this' FIRST (which is in local slot 0)
                        program.emit(bytecode::OpCode::LOAD_LOCAL, 0);

                        // Compile the initializer expression (pushes value)
                        fieldNode->getInitialValue()->accept(*this);

                        // Store in field (SET_FIELD expects stack: [object, value] with value on top, pops value first then object)
                        std::string fieldName = fieldNode->getName();
                        size_t fieldNameIndex = program.getConstantPool().addString(fieldName);
                        program.emit(bytecode::OpCode::SET_FIELD, static_cast<uint32_t>(fieldNameIndex));
                    }
                }
            }

            // Return 'this'
            program.emit(bytecode::OpCode::LOAD_LOCAL, 0);
            program.emit(bytecode::OpCode::RETURN_VALUE);

            size_t localCount = getLocalCount();
            endScope();
            exitFunctionFrame();

            size_t constructorEnd = program.getCurrentOffset();
            patchJump(skipJump);

            // Register default constructor with proper naming: ClassName::<init>/0
            // For generic classes, register with base name for runtime instantiation
            std::string className = node->getClassName();
            std::string constructorName = className + "::<init>/0";  // 0 args (only 'this' is implicit)

            bytecode::BytecodeProgram::FunctionMetadata metadata;
            metadata.name = constructorName;
            metadata.startOffset = constructorStart;
            metadata.instructionCount = constructorEnd - constructorStart;
            metadata.localCount = localCount;
            metadata.parameterCount = 1;  // Just 'this'
            metadata.parameterNames = paramNames;
            metadata.returnType = "object";
            metadata.isStatic = false;
            metadata.isNative = false;

            program.registerFunction(metadata.name, metadata);

            // If this is a generic class, also register with full generic name
            if (node->isGeneric()) {
                std::string fullClassName = node->getFullClassName();
                std::string genericConstructorName = fullClassName + "::<init>/0";
                metadata.name = genericConstructorName;
                program.registerFunction(genericConstructorName, metadata);
            }
        }

        // Compile instance methods
        int methodIndex = 0;
        for (const auto& method : node->getMethods()) {
            if (auto* methodNode = dynamic_cast<ast::MethodNode*>(method.get())) {
                if (!methodNode->getIsStatic()) {
                    method->accept(*this);
                }
            } else {
            }
            methodIndex++;
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
            try {
                body->accept(*this);
            } catch (const std::exception&) {
                throw;
            } catch (...) {
                throw;
            }
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

        auto params = node->getParameters();

        // Set instance method context (constructors are like instance methods)
        bool wasInInstanceMethod = inInstanceMethod;
        inInstanceMethod = true;

        // Emit JUMP to skip over constructor body during main execution
        size_t skipJump = emitJump(bytecode::OpCode::JUMP);

        // Constructor starts here
        size_t constructorStart = program.getCurrentOffset();

        // Get parameters
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

        // Initialize instance fields with their default values (before constructor body)
        // This ensures fields are initialized even if not explicitly set in constructor
        if (currentClassNode) {
            auto& fields = currentClassNode->getFields();
            for (const auto& fieldPtr : fields) {
                if (auto* fieldNode = dynamic_cast<ast::FieldNode*>(fieldPtr.get())) {
                    // Only initialize instance fields (not static)
                    if (!fieldNode->getIsStatic() && fieldNode->getInitialValue()) {
                        // Load 'this' FIRST (which is in local slot 0)
                        program.emit(bytecode::OpCode::LOAD_LOCAL, 0);

                        // Compile the initializer expression (pushes value)
                        fieldNode->getInitialValue()->accept(*this);

                        // Store in field (SET_FIELD expects stack: [object, value] with value on top, pops value first then object)
                        std::string fieldName = fieldNode->getName();
                        size_t fieldNameIndex = program.getConstantPool().addString(fieldName);
                        program.emit(bytecode::OpCode::SET_FIELD, static_cast<uint32_t>(fieldNameIndex));
                    }
                }
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
        // For generic classes, register constructor with BOTH the full generic name AND the base name
        // Full name (e.g., "LinkedList<T>::<init>/0") for compile-time type checking
        // Base name (e.g., "LinkedList::<init>/0") for runtime instantiation
        std::string className = currentClassNode ? currentClassNode->getClassName() : "";
        std::string constructorName = className + "::<init>/" + std::to_string(params.size());

        program.registerFunction(constructorName, metadata);

        // If this is a generic class, also register with full generic name for type checking
        if (currentClassNode && currentClassNode->isGeneric()) {
            std::string fullClassName = currentClassNode->getFullClassName();
            std::string genericConstructorName = fullClassName + "::<init>/" + std::to_string(params.size());
            program.registerFunction(genericConstructorName, metadata);
        }

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

                // Parse individual type arguments
                size_t start = 0;
                size_t depth = 0;
                for (size_t i = 0; i < typeArgsStr.length(); ++i) {
                    if (typeArgsStr[i] == '<') depth++;
                    else if (typeArgsStr[i] == '>') depth--;
                    else if (typeArgsStr[i] == ',' && depth == 0) {
                        std::string arg = typeArgsStr.substr(start, i - start);
                        // Trim whitespace
                        arg.erase(0, arg.find_first_not_of(" \t"));
                        arg.erase(arg.find_last_not_of(" \t") + 1);
                        typeArguments.push_back(arg);
                        start = i + 1;
                    }
                }
                // Add last argument
                std::string arg = typeArgsStr.substr(start);
                arg.erase(0, arg.find_first_not_of(" \t"));
                arg.erase(arg.find_last_not_of(" \t") + 1);
                if (!arg.empty()) {
                    typeArguments.push_back(arg);
                }

                // Validate that type arguments are not primitive types
                static const std::unordered_set<std::string> primitiveTypes = {
                    "int", "float", "bool", "string", "void"
                };

                for (const auto& typeArg : typeArguments) {
                    if (primitiveTypes.find(typeArg) != primitiveTypes.end()) {
                        throw errors::TypeException(
                            "Generic type arguments cannot be primitive types. Use wrapper classes instead (Int, Float, Bool, String). Found: " + typeArg,
                            node->getLocation()
                        );
                    }
                }
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

            // Check if this is array.length access
            if (memberName == "length") {
                // Special case: array.length should use ARRAY_LENGTH opcode
                emitWithLocation(bytecode::OpCode::ARRAY_LENGTH, node);
            } else {
                // Regular field access - emit GET_FIELD instruction
                size_t fieldNameIndex = program.getConstantPool().addString(memberName);
                emitWithLocation(bytecode::OpCode::GET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);
            }
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
            // Static method call: ClassName::methodName(args) or this::methodName(args)
            // Push all arguments onto stack
            for (const auto& arg : arguments) {
                arg->accept(*this);
            }

            // Build fully qualified method name
            std::string className;
            auto* objectNode = node->getObject();
            if (auto* varNode = dynamic_cast<ast::VariableNode*>(objectNode)) {
                className = varNode->getName();
                // If using "this::", replace with actual class name
                if (className == "this" && currentClassNode) {
                    className = currentClassNode->getClassName();
                }
            }
            std::string qualifiedName = className + "::" + methodName;

            // Emit CALL_STATIC instruction with fully qualified name
            size_t methodNameIndex = program.getConstantPool().addString(qualifiedName);
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
        // Push arguments onto stack
        // Note: 'this' is NOT pushed here - the VM will get it from the current frame's slot 0
        const auto& arguments = node->getArguments();
        for (const auto& arg : arguments) {
            arg->accept(*this);
        }

        // Emit SUPER_CONSTRUCTOR instruction with current class name
        // The VM needs to know which class's parent constructor to call
        std::string currentClassName = currentClassNode ? currentClassNode->getClassName() : "";
        size_t classNameIndex = program.getConstantPool().addString(currentClassName);
        program.emit(bytecode::OpCode::SUPER_CONSTRUCTOR,
                     static_cast<uint32_t>(classNameIndex),
                     static_cast<uint32_t>(arguments.size()));

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitSuperMethodCallNode(ast::SuperMethodCallNode* node)
    {
        std::string methodName = node->getMethodName();

        // Push arguments onto stack
        // Note: 'this' is NOT pushed here - the VM will get it from the current frame's slot 0
        const auto& arguments = node->getArguments();
        for (const auto& arg : arguments) {
            arg->accept(*this);
        }

        // Emit SUPER_INVOKE instruction with current class name
        // The VM needs to know which class's parent method to call
        std::string currentClassName = currentClassNode ? currentClassNode->getClassName() : "";
        size_t classNameIndex = program.getConstantPool().addString(currentClassName);
        size_t methodNameIndex = program.getConstantPool().addString(methodName);
        program.emit(bytecode::OpCode::SUPER_INVOKE, std::vector<uint32_t>{
            static_cast<uint32_t>(classNameIndex),
            static_cast<uint32_t>(methodNameIndex),
            static_cast<uint32_t>(arguments.size())
        });

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
            // Count and push only non-null dimension sizes onto stack
            // For jagged arrays like int[5][], sizeExpressions contains [expr, nullptr]
            size_t specifiedDimensions = 0;
            for (const auto& sizeExpr : sizeExpressions) {
                if (sizeExpr) {
                    sizeExpr->accept(*this);
                    specifiedDimensions++;
                }
            }

            // Get element type info
            const auto& typeInfo = node->getElementTypeInfo();
            size_t typeNameIndex = program.getConstantPool().addString(typeInfo.toString());

            // Emit NEW_ARRAY_MULTI with element type and number of specified dimensions
            // operands: [typeNameIndex, specifiedDimensions, totalDimensions]
            std::vector<uint32_t> operands = {
                static_cast<uint32_t>(typeNameIndex),
                static_cast<uint32_t>(specifiedDimensions),
                static_cast<uint32_t>(dimensionCount)
            };
            program.emit(bytecode::OpCode::NEW_ARRAY_MULTI, operands);
        }

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitArrayLiteralNode(ast::ArrayLiteralNode* node)
    {
        const auto& elements = node->getElements();
        size_t elementCount = elements.size();

        // Type validation - check that all elements have compatible types
        if (elementCount > 0) {
            std::string expectedType;
            bool isFirstElement = true;

            for (size_t i = 0; i < elementCount; ++i) {
                std::string currentType;

                // Determine type of current element
                if (dynamic_cast<ast::IntegerNode*>(elements[i].get())) {
                    currentType = "int";
                } else if (dynamic_cast<ast::FloatNode*>(elements[i].get())) {
                    currentType = "float";
                } else if (dynamic_cast<ast::StringNode*>(elements[i].get())) {
                    currentType = "string";
                } else if (dynamic_cast<ast::BoolNode*>(elements[i].get())) {
                    currentType = "bool";
                } else if (dynamic_cast<ast::NullNode*>(elements[i].get())) {
                    currentType = "null";
                } else if (auto* newNode = dynamic_cast<ast::NewNode*>(elements[i].get())) {
                    // Object type - use class name
                    currentType = "object:" + newNode->getClassName();
                } else {
                    // For complex expressions, we can't determine type at compile time
                    currentType = "unknown";
                }

                if (isFirstElement) {
                    expectedType = currentType;
                    isFirstElement = false;
                } else {
                    // Validate type compatibility
                    if (currentType != "unknown" && expectedType != "unknown") {
                        // Special case: allow int in float arrays
                        bool compatible = (currentType == expectedType) ||
                                        (expectedType == "float" && currentType == "int") ||
                                        (currentType == "null" && expectedType.find("object:") == 0) ||
                                        (expectedType == "null" && currentType.find("object:") == 0);

                        if (!compatible) {
                            // For better error messages, extract class names from object types
                            std::string expectedTypeName = expectedType;
                            std::string currentTypeName = currentType;

                            if (expectedType.find("object:") == 0) {
                                expectedTypeName = expectedType.substr(7); // Remove "object:" prefix
                            }
                            if (currentType.find("object:") == 0) {
                                currentTypeName = currentType.substr(7); // Remove "object:" prefix
                            }

                            throw errors::TypeException(
                                "Array literal type mismatch: expected " + expectedTypeName +
                                " but found " + currentTypeName,
                                node->getLocation()
                            );
                        }
                    }
                }
            }
        }

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
