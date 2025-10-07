#include "StatementCompiler.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/EnvironmentException.hpp"
#include "../../../errors/UndefinedException.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/LambdaNode.hpp"


namespace vm::compiler::visitors
{
    StatementCompiler::StatementCompiler(CompilerContext& context)
        : ctx(context)
    {
    }

    value::Value StatementCompiler::compileDeclaration(ast::DeclarationNode* node)
    {
        std::string name = node->getVariableName();
        auto* initializer = node->getInitializer();

        value::ValueType valueType = value::ValueType::VOID;
        try {
            valueType = node->getType();
        } catch (...) {
            valueType = value::ValueType::OBJECT;
        }

        // Lambda validation for declarations
        if (initializer && dynamic_cast<ast::LambdaNode*>(initializer)) {
            // Check if assigning lambda to functional interface
            if (valueType == value::ValueType::OBJECT) {
                // Infer the class name from the initializer
                std::string className = ctx.typeInference.inferExpressionClassName(initializer);
                if (!className.empty()) {
                    // Validate that the interface is functional (has exactly one method)
                    auto interfaceDef = ctx.environment->findInterface(className);
                    if (interfaceDef && !interfaceDef->isFunctionalInterface()) {
                        auto methodSignatures = interfaceDef->getMethodSignatures();
                        throw errors::TypeException(
                            "Cannot assign lambda to non-functional interface '" + className + "'. " +
                            "Lambdas can only be assigned to interfaces with exactly one method. " +
                            "Interface '" + className + "' has " + std::to_string(methodSignatures.size()) + " methods. " +
                            "Consider using a functional interface (single method) or implement the interface explicitly.",
                            node->getLocation()
                        );
                    }
                }
            }
        }

        // Compile the initializer (if any)
        if (initializer) {
            initializer->accept(ctx.visitor);  // Will need delegation
        } else {
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
        }

        // Track local variable if we're in a function
        if (ctx.functionFrameManager.isInFunction()) {
            // Check for variable redefinition
            if (ctx.variableTracker.existsInCurrentScope(name)) {
                throw errors::EnvironmentException(
                    "Variable '" + name + "' is already defined in this scope",
                    node->getLocation()
                );
            }

            // Declare local variable
            std::string className = (valueType == value::ValueType::OBJECT && initializer) ?
                ctx.typeInference.inferExpressionClassName(initializer) : "";
            ctx.variableTracker.declareLocal(name, valueType, className);

            // Update max local slot
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

            // Emit STORE_LOCAL
            size_t slot = ctx.variableTracker.getNextLocalSlot() - 1;
            size_t nameIndex = ctx.program.getConstantPool().addString(name);
            ctx.program.emit(bytecode::OpCode::STORE_LOCAL,
                           std::vector<uint32_t>{
                               static_cast<uint32_t>(slot),
                               static_cast<uint32_t>(nameIndex)
                           });

            return std::monostate{};
        }

        // Global variable - check for redefinition in current scope
        if (ctx.globalRegistry.existsInCurrentScope(name, ctx.variableTracker.getCurrentScopeDepth())) {
            throw errors::EnvironmentException(
                "Variable '" + name + "' is already defined in this scope",
                node->getLocation()
            );
        }

        std::string className = (valueType == value::ValueType::OBJECT && initializer) ?
            ctx.typeInference.inferExpressionClassName(initializer) : "";
        ctx.globalRegistry.registerGlobal(name, node->getType(), className,
                                         ctx.variableTracker.getCurrentScopeDepth());

        size_t nameIndex = ctx.program.getConstantPool().addString(name);
        size_t typeIndex = ctx.program.getConstantPool().addString("auto");
        uint32_t isFinal = node->isFinal() ? 1 : 0;

        ctx.program.emit(bytecode::OpCode::DECLARE_VAR,
                       std::vector<uint32_t>{
                           static_cast<uint32_t>(nameIndex),
                           static_cast<uint32_t>(typeIndex),
                           isFinal
                       });

        return std::monostate{};
    }

    value::Value StatementCompiler::compileAssignment(ast::AssignmentNode* node)
    {
        std::string name = node->getVariableName();
        value::ValueType varType = node->getVariableType();
        auto* value = node->getValue();

        // Check if this is a reassignment (variable already exists) or a declaration
        bool isReassignment = false;
        std::string existingClassName;

        if (ctx.functionFrameManager.isInFunction()) {
            existingClassName = ctx.variableTracker.getLocalClassNameByName(name);
            if (!existingClassName.empty() || ctx.variableTracker.existsInCurrentScope(name)) {
                isReassignment = true;
            }
        }

        if (!isReassignment && ctx.globalRegistry.exists(name)) {
            isReassignment = true;
            existingClassName = ctx.globalRegistry.getClassName(name);
        }

        // Type checking for class/interface existence
        if (varType == value::ValueType::OBJECT && !node->getClassName().empty()) {
            std::string className = node->getClassName();
            bool isArrayType = className.find("[]") != std::string::npos;
            bool isGenericParam = (className.length() == 1 && std::isupper(className[0]));

            if (!isArrayType && !isGenericParam) {
                std::string baseClassName = ctx.genericResolver.extractBaseTypeName(className);

                if (!ctx.environment->findClass(baseClassName)) {
                    const auto& classes = ctx.program.getClasses();
                    bool found = false;
                    for (const auto& classMeta : classes) {
                        if (classMeta.name == baseClassName) {
                            found = true;
                            break;
                        }
                    }
                    if (!found && !ctx.environment->findInterface(baseClassName)) {
                        throw errors::UndefinedException(
                            "Undefined class or interface: '" + baseClassName + "'",
                            node->getLocation()
                        );
                    }
                }
            }
        }

        // Lambda validation
        if (value && dynamic_cast<ast::LambdaNode*>(value)) {
            // Check if assigning lambda to functional interface (for declarations)
            if (!isReassignment && varType == value::ValueType::OBJECT && !node->getClassName().empty()) {
                // Validate that the interface is functional (has exactly one method)
                auto interfaceDef = ctx.environment->findInterface(node->getClassName());
                if (interfaceDef && !interfaceDef->isFunctionalInterface()) {
                    auto methodSignatures = interfaceDef->getMethodSignatures();
                    throw errors::TypeException(
                        "Cannot assign lambda to non-functional interface '" + node->getClassName() + "'. " +
                        "Lambdas can only be assigned to interfaces with exactly one method. " +
                        "Interface '" + node->getClassName() + "' has " + std::to_string(methodSignatures.size()) + " methods. " +
                        "Consider using a functional interface (single method) or implement the interface explicitly.",
                        node->getLocation()
                    );
                }
            }

            // Lambda reassignment validation
            if (isReassignment) {
                // If the variable is an interface type, reject reassignment
                if (!existingClassName.empty() && ctx.environment->findInterface(existingClassName)) {
                    throw errors::TypeException(
                        "Type mismatch for variable '" + name + "': expected object but got void",
                        node->getLocation()
                    );
                }
            }
        }

        // Type compatibility validation for reassignments
        if (isReassignment && !existingClassName.empty() && value) {
            std::string valueClassName = ctx.typeInference.inferExpressionClassName(value);
            value::ValueType valueType = ctx.typeInference.inferExpressionType(value);
            bool isNullValue = dynamic_cast<ast::NullNode*>(value) != nullptr;

            // Only validate if we have actual type information
            if (!valueClassName.empty() || valueType != value::ValueType::OBJECT) {
                // Validate that the assigned value is compatible with the variable's type
                ctx.typeValidator.validateAssignment(value::ValueType::OBJECT, existingClassName,
                                                    valueType, valueClassName, isNullValue, node->getLocation());
            }
        }

        // Compile the value
        if (value) {
            // Type validation for declarations
            if (varType != value::ValueType::VOID) {
                value::ValueType valueType = ctx.typeInference.inferExpressionType(value);
                std::string varClassName = node->getClassName();
                std::string valueClassName = ctx.typeInference.inferExpressionClassName(value);
                bool isNullValue = dynamic_cast<ast::NullNode*>(value) != nullptr;

                ctx.typeValidator.validateAssignment(varType, varClassName, valueType,
                                                    valueClassName, isNullValue, node->getLocation());
            }
            value->accept(ctx.visitor);  // Will need delegation
        } else {
            ctx.program.emit(bytecode::OpCode::PUSH_NULL);
        }

        // Check if this is a declaration or pure assignment
        if (varType != value::ValueType::VOID) {
            // Declaration with initializer
            if (ctx.functionFrameManager.isInFunction() && ctx.variableTracker.getCurrentScopeDepth() > 0) {
                // Check if variable exists anywhere in the function (including parameters)
                // This prevents parameter shadowing and variable redefinition
                if (ctx.variableTracker.existsInFunction(name)) {
                    throw errors::EnvironmentException(
                        "Variable '" + name + "' is already defined in this scope",
                        node->getLocation()
                    );
                }

                ctx.variableTracker.declareLocal(name, varType, node->getClassName());
                ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

                ctx.emitter.emitWithLocation(bytecode::OpCode::DUP, node);
                size_t slot = ctx.variableTracker.getNextLocalSlot() - 1;
                size_t nameIndex = ctx.program.getConstantPool().addString(name);
                ctx.program.emit(bytecode::OpCode::STORE_LOCAL,
                               static_cast<uint32_t>(slot),
                               static_cast<uint32_t>(nameIndex));
                return std::monostate{};
            }

            // Global variable declaration - check for redefinition
            if (ctx.globalRegistry.existsInCurrentScope(name, ctx.variableTracker.getCurrentScopeDepth())) {
                throw errors::EnvironmentException(
                    "Variable '" + name + "' is already defined in this scope",
                    node->getLocation()
                );
            }

            ctx.globalRegistry.registerGlobal(name, varType, node->getClassName(),
                                             ctx.variableTracker.getCurrentScopeDepth());

            size_t nameIndex = ctx.program.getConstantPool().addString(name);
            size_t typeIndex = ctx.program.getConstantPool().addString("auto");
            uint32_t isFinal = node->getIsFinal() ? 1 : 0;

            ctx.program.emit(bytecode::OpCode::DECLARE_VAR,
                           std::vector<uint32_t>{
                               static_cast<uint32_t>(nameIndex),
                               static_cast<uint32_t>(typeIndex),
                               isFinal
                           });
        } else {
            // Pure assignment - check if it's a local or global
            if (name.find("::") != std::string::npos) {
                size_t nameIndex = ctx.program.getConstantPool().addString(name);
                ctx.emitter.emitWithLocation(bytecode::OpCode::SET_STATIC, static_cast<uint32_t>(nameIndex), node);
                return std::monostate{};
            }

            // Check for local variable
            size_t localSlot = SIZE_MAX;
            if (ctx.functionFrameManager.isInFunction()) {
                localSlot = ctx.variableTracker.resolveLocal(name,
                    ctx.functionFrameManager.currentFrame().localStartSlot);
            }

            if (localSlot != SIZE_MAX) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::DUP, node);
                size_t nameIndex = ctx.program.getConstantPool().addString(name);
                ctx.program.emit(bytecode::OpCode::STORE_LOCAL,
                               static_cast<uint32_t>(localSlot),
                               static_cast<uint32_t>(nameIndex));
                return std::monostate{};
            }

            // Check if it's an instance field (when inside a method)
            if (ctx.currentClassNode) {
                auto classRegistry = ctx.environment->getClassRegistry();
                if (classRegistry) {
                    auto classDef = classRegistry->findClass(ctx.currentClassNode->getClassName());
                    if (classDef) {
                        const auto& instanceFields = classDef->getInstanceFields();
                        if (instanceFields.find(name) != instanceFields.end()) {
                            // It's an instance field - emit: LOAD_LOCAL 0 (this), value, SET_FIELD
                            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
                            ctx.emitter.emitWithLocation(bytecode::OpCode::SWAP, node);
                            size_t fieldNameIndex = ctx.program.getConstantPool().addString(name);
                            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD, static_cast<uint32_t>(fieldNameIndex), node);
                            return std::monostate{};
                        }
                    }
                }
            }

            // Global variable assignment
            size_t nameIndex = ctx.program.getConstantPool().addString(name);
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_VAR, static_cast<uint32_t>(nameIndex), node);
        }

        return std::monostate{};
    }

    value::Value StatementCompiler::compileBlock(ast::BlockNode* node)
    {
        ctx.variableTracker.beginScope();
        const auto& statements = node->getStatements();
        for (auto& stmt : statements) {
            stmt->accept(ctx.visitor);  // Will need delegation
        }
        ctx.variableTracker.endScope();
        ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());
        return std::monostate{};
    }

    value::Value StatementCompiler::compileProgram(ast::ProgramNode* node)
    {
        const auto& statements = node->getStatements();
        for (auto& stmt : statements) {
            stmt->accept(ctx.visitor);  // Will need delegation
        }
        return std::monostate{};
    }
}
