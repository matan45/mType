#include "StatementCompiler.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/EnvironmentException.hpp"
#include "../../../errors/UndefinedException.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/LambdaNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
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
                std::string baseClassName = ctx.genericResolver.extractBaseTypeName(className);

                // Validate that generic types have type arguments
                if (ctx.compileTimeValidator)
                {
                    ctx.compileTimeValidator->validateTypeIsNotRawGeneric(className, node->getLocation());
                }

                // Skip validation for Promise types (native async/await support)
                if (baseClassName != "Promise")
                {
                    if (!ctx.environment->findClass(baseClassName))
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
                        if (!found && !ctx.environment->findInterface(baseClassName))
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

    void StatementCompiler::validateLambdaAssignment(ast::AssignmentNode* node, bool isReassignment,
                                                     const std::string& existingClassName)
    {
        auto* value = node->getValue();
        if (!value || !dynamic_cast<ast::LambdaNode*>(value))
        {
            return;
        }

        value::ValueType varType = node->getVariableType();

        // Check if assigning lambda to functional interface (for declarations)
        if (!isReassignment && varType == value::ValueType::OBJECT && !node->getClassName().empty())
        {
            // Validate that the interface is functional (has exactly one method)
            auto interfaceDef = ctx.environment->findInterface(node->getClassName());
            if (interfaceDef && !interfaceDef->isFunctionalInterface())
            {
                auto methodSignatures = interfaceDef->getMethodSignatures();
                throw errors::TypeException(
                    "Cannot assign lambda to non-functional interface '" + node->getClassName() + "'. " +
                    "Lambdas can only be assigned to interfaces with exactly one method. " +
                    "Interface '" + node->getClassName() + "' has " + std::to_string(methodSignatures.size()) +
                    " methods. " +
                    "Consider using a functional interface (single method) or implement the interface explicitly.",
                    node->getLocation()
                );
            }
        }

        // Lambda reassignment validation
        if (isReassignment)
        {
            // If the variable is an interface type, reject reassignment
            if (!existingClassName.empty() && ctx.environment->findInterface(existingClassName))
            {
                throw errors::TypeException(
                    "Type mismatch for variable '" + node->getVariableName() + "': expected object but got void",
                    node->getLocation()
                );
            }
        }
    }

    void StatementCompiler::validateReassignmentType(ast::AssignmentNode* node, const std::string& existingClassName)
    {
        auto* value = node->getValue();
        if (existingClassName.empty() || !value)
        {
            return;
        }

        std::string valueClassName = ctx.typeInference.inferExpressionClassName(value);
        value::ValueType valueType = ctx.typeInference.inferExpressionType(value);
        bool isNullValue = dynamic_cast<ast::NullNode*>(value) != nullptr;

        // Only validate if we have actual type information
        if (!valueClassName.empty() || valueType != value::ValueType::OBJECT)
        {
            // Resolve generic type parameters if present
            std::string resolvedExistingClassName = ctx.resolveGenericType(existingClassName);
            std::string resolvedValueClassName = ctx.resolveGenericType(valueClassName);

            // Validate that the assigned value is compatible with the variable's type
            ctx.typeValidator.validateAssignment(value::ValueType::OBJECT, resolvedExistingClassName,
                                                 valueType, resolvedValueClassName, isNullValue, node->getLocation());
        }
    }

    void StatementCompiler::emitVariableDeclaration(ast::AssignmentNode* node)
    {
        std::string name = node->getVariableName();
        value::ValueType varType = node->getVariableType();

        if (ctx.functionFrameManager.isInFunction())
        {
            // Check if variable exists in the current scope only
            // This prevents redefinition in the same scope but allows shadowing in nested scopes
            if (ctx.variableTracker.existsInCurrentScope(name))
            {
                throw errors::EnvironmentException(
                    "Variable '" + name + "' is already defined in this scope",
                    node->getLocation()
                );
            }

            // If in a lambda, prevent shadowing of captured outer variables (C# behavior)
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

            ctx.variableTracker.declareLocal(name, varType, node->getClassName());
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

            // STORE_LOCAL will consume the value from the stack - no DUP needed
            // Convert absolute slot to relative slot within this function frame
            size_t absoluteSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            size_t relativeSlot = absoluteSlot - ctx.functionFrameManager.currentFrame().localStartSlot;
            size_t nameIndex = ctx.program.getConstantPool().addString(name);
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL,
                                         static_cast<uint32_t>(relativeSlot),
                                         static_cast<uint32_t>(nameIndex), node);
            return;
        }

        // Global variable declaration - check for redefinition
        if (ctx.globalRegistry.existsInCurrentScope(name, ctx.variableTracker.getCurrentScopeDepth()))
        {
            throw errors::EnvironmentException(
                "Variable '" + name + "' is already defined in this scope",
                node->getLocation()
            );
        }

        ctx.globalRegistry.registerGlobal(name, varType, node->getClassName(),
                                          ctx.variableTracker.getCurrentScopeDepth());

        // Register global variable metadata for debugger
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
                                     std::vector<uint32_t>{
                                         static_cast<uint32_t>(nameIndex),
                                         static_cast<uint32_t>(typeIndex),
                                         isFinal
                                     }, node);
    }

    void StatementCompiler::emitVariableReassignment(ast::AssignmentNode* node, bool isReassignment)
    {
        std::string name = node->getVariableName();
        value::ValueType varType = node->getVariableType();

        // Check for static field assignment
        if (name.find("::") != std::string::npos)
        {
            size_t nameIndex = ctx.program.getConstantPool().addString(name);
            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_STATIC, static_cast<uint32_t>(nameIndex), node);
            return;
        }

        // Check for local variable
        size_t localSlot = SIZE_MAX;
        if (ctx.functionFrameManager.isInFunction())
        {
            localSlot = ctx.variableTracker.resolveLocal(name,
                                                         ctx.functionFrameManager.currentFrame().localStartSlot);
        }

        // If found as existing local, store to it
        if (localSlot != SIZE_MAX)
        {
            // STORE_LOCAL will consume the value from the stack - no DUP needed
            size_t nameIndex = ctx.program.getConstantPool().addString(name);
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL,
                                         static_cast<uint32_t>(localSlot),
                                         static_cast<uint32_t>(nameIndex), node);
            return;
        }

        // IMPORTANT: Check if it's an instance field BEFORE treating as type-inferred local
        // This prevents fields from being incorrectly registered as local variables
        // During bytecode compilation, we check the AST ClassNode directly instead of
        // querying the runtime classRegistry (which isn't populated yet at compile-time)
        if (ctx.currentClassNode && ctx.functionFrameManager.isInFunction())
        {
            // Check fields in the current class's AST
            const auto& fields = ctx.currentClassNode->getFields();
            for (const auto& fieldPtr : fields)
            {
                if (auto* fieldNode = dynamic_cast<ast::FieldNode*>(fieldPtr.get()))
                {
                    if (fieldNode->getName() == name)
                    {
                        if (fieldNode->getIsStatic())
                        {
                            // Static field - use fully qualified name: ClassName::fieldName
                            std::string qualifiedName = ctx.currentClassNode->getClassName() + "::" + name;
                            size_t nameIndex = ctx.program.getConstantPool().addString(qualifiedName);
                            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_STATIC, static_cast<uint32_t>(nameIndex),
                                                         node);
                            return;
                        }
                        else
                        {
                            // Instance field - emit: LOAD_LOCAL 0 (this), value, SET_FIELD
                            ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
                            ctx.emitter.emitWithLocation(bytecode::OpCode::SWAP, node);
                            size_t fieldNameIndex = ctx.program.getConstantPool().addString(name);
                            ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD,
                                                         static_cast<uint32_t>(fieldNameIndex), node);
                            return;
                        }
                    }
                }
            }

            // Check parent class fields for inheritance
            // Try to use class registry if available (classes may be pre-registered)
            if (ctx.currentClassNode->hasParentClass())
            {
                auto classRegistry = ctx.environment->getClassRegistry();
                if (classRegistry)
                {
                    std::string parentClassName = ctx.currentClassNode->getParentClassName();
                    auto parentClass = classRegistry->findClass(parentClassName);

                    // Walk up the inheritance chain
                    while (parentClass)
                    {
                        auto field = parentClass->getField(name);
                        if (field)
                        {
                            if (field->isStatic())
                            {
                                // Static field - use fully qualified name with the class where it's defined
                                std::string qualifiedName = parentClass->getName() + "::" + name;
                                size_t nameIndex = ctx.program.getConstantPool().addString(qualifiedName);
                                ctx.emitter.emitWithLocation(bytecode::OpCode::SET_STATIC,
                                                             static_cast<uint32_t>(nameIndex), node);
                                return;
                            }
                            else
                            {
                                // Inherited instance field - emit: LOAD_LOCAL 0 (this), value, SET_FIELD
                                ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
                                ctx.emitter.emitWithLocation(bytecode::OpCode::SWAP, node);
                                size_t fieldNameIndex = ctx.program.getConstantPool().addString(name);
                                ctx.emitter.emitWithLocation(bytecode::OpCode::SET_FIELD,
                                                             static_cast<uint32_t>(fieldNameIndex), node);
                                return;
                            }
                        }

                        // Continue up the inheritance chain
                        parentClass = parentClass->hasParentClass() ? parentClass->getParentClass() : nullptr;
                    }
                }
            }
        }

        // If we're in a function and variable doesn't exist yet and this is not a reassignment,
        // it's a new local variable declaration with type inference - register it
        if (ctx.functionFrameManager.isInFunction() && !isReassignment)
        {
            // Check if variable exists in the current scope only
            // This prevents redefinition in the same scope but allows shadowing in nested scopes
            if (ctx.variableTracker.existsInCurrentScope(name))
            {
                throw errors::EnvironmentException(
                    "Variable '" + name + "' is already defined in this scope",
                    node->getLocation()
                );
            }

            // If in a lambda, prevent shadowing of captured outer variables (C# behavior)
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

            ctx.variableTracker.declareLocal(name, varType, node->getClassName());
            ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

            // STORE_LOCAL will consume the value from the stack - no DUP needed
            // Convert absolute slot to relative slot within this function frame
            size_t absoluteSlot = ctx.variableTracker.getNextLocalSlot() - 1;
            size_t relativeSlot = absoluteSlot - ctx.functionFrameManager.currentFrame().localStartSlot;
            size_t nameIndex = ctx.program.getConstantPool().addString(name);
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL,
                                         static_cast<uint32_t>(relativeSlot),
                                         static_cast<uint32_t>(nameIndex), node);
            return;
        }

        // Global variable assignment - validate variable exists
        if (!ctx.globalRegistry.exists(name))
        {
            throw errors::UndefinedException(
                "Cannot assign to undeclared variable '" + name + "'. "
                "Did you forget to declare it with a type?",
                node->getLocation()
            );
        }

        size_t nameIndex = ctx.program.getConstantPool().addString(name);
        ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_VAR, static_cast<uint32_t>(nameIndex), node);
    }

    value::Value StatementCompiler::compileAssignment(ast::AssignmentNode* node)
    {
        std::string name = node->getVariableName();
        value::ValueType varType = node->getVariableType();
        auto* value = node->getValue();

        // Detect whether this is a reassignment or new declaration
        std::string existingClassName;
        bool isReassignment = detectReassignment(name, existingClassName);

        // Validate class/interface type exists
        validateClassOrInterfaceType(node);

        // Validate lambda assignments
        validateLambdaAssignment(node, isReassignment, existingClassName);

        // Type compatibility validation for reassignments
        if (isReassignment)
        {
            validateReassignmentType(node, existingClassName);
        }

        // Compile the value expression
        if (value)
        {
            // Type validation for declarations
            if (varType != value::ValueType::VOID)
            {
                value::ValueType valueType = ctx.typeInference.inferExpressionType(value);
                std::string varClassName = ctx.resolveGenericType(node->getClassName());
                std::string valueClassName = ctx.resolveGenericType(ctx.typeInference.inferExpressionClassName(value));
                bool isNullValue = dynamic_cast<ast::NullNode*>(value) != nullptr;

                ctx.typeValidator.validateAssignment(varType, varClassName, valueType,
                                                     valueClassName, isNullValue, node->getLocation());
            }
            value->accept(ctx.visitor);
        }
        else
        {
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
        }

        // Emit bytecode based on whether this is a declaration or reassignment
        if (varType != value::ValueType::VOID)
        {
            emitVariableDeclaration(node);
        }
        else
        {
            emitVariableReassignment(node, isReassignment);
        }

        return std::monostate{};
    }

    value::Value StatementCompiler::compileBlock(ast::BlockNode* node)
    {
        // IMPORTANT: Function body blocks should NOT create their own scope!
        // The function already creates a scope, and if we create another one here,
        // all variables will be cleared before FunctionCompiler can capture their names.
        bool shouldManageScope = !ctx.functionFrameManager.isInFunction();

        if (shouldManageScope)
        {
            ctx.variableTracker.beginScope();
        }

        const auto& statements = node->getStatements();
        for (auto& stmt : statements)
        {
            stmt->accept(ctx.visitor); // Will need delegation
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
        // Script-level code - variables are globals, not locals
        // Functions and lambdas can access these global variables
        ctx.variableTracker.beginScope();

        const auto& statements = node->getStatements();
        for (auto& stmt : statements)
        {
            stmt->accept(ctx.visitor); // Will need delegation
        }

        ctx.variableTracker.endScope();
        ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

        return std::monostate{};
    }
}
