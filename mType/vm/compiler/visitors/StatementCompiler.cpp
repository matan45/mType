#include "StatementCompiler.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/EnvironmentException.hpp"
#include "../../../errors/UndefinedException.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/LambdaNode.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/NewNode.hpp"
#include "../validation/CompileTimeValidator.hpp"
#include  <iostream>
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

    std::vector<ast::nodes::functions::ReturnNode*> StatementCompiler::collectReturnStatements(ast::ASTNode* node)
    {
        std::vector<ast::nodes::functions::ReturnNode*> returns;

        if (!node)
        {
            return returns;
        }

        // Check if this node is a return statement
        if (auto* returnNode = dynamic_cast<ast::nodes::functions::ReturnNode*>(node))
        {
            returns.push_back(returnNode);
            return returns;
        }

        // Recursively traverse block statements
        if (auto* blockNode = dynamic_cast<ast::nodes::statements::BlockNode*>(node))
        {
            for (const auto& stmt : blockNode->getStatements())
            {
                auto childReturns = collectReturnStatements(stmt.get());
                returns.insert(returns.end(), childReturns.begin(), childReturns.end());
            }
        }
        // Traverse if statements
        else if (auto* ifNode = dynamic_cast<ast::nodes::statements::IfNode*>(node))
        {
            auto thenReturns = collectReturnStatements(ifNode->getThenStatement());
            returns.insert(returns.end(), thenReturns.begin(), thenReturns.end());

            if (ifNode->getElseStatement())
            {
                auto elseReturns = collectReturnStatements(ifNode->getElseStatement());
                returns.insert(returns.end(), elseReturns.begin(), elseReturns.end());
            }
        }
        // Traverse while loops
        else if (auto* whileNode = dynamic_cast<ast::nodes::statements::WhileNode*>(node))
        {
            auto bodyReturns = collectReturnStatements(whileNode->getBody());
            returns.insert(returns.end(), bodyReturns.begin(), bodyReturns.end());
        }
        // Traverse for loops
        else if (auto* forNode = dynamic_cast<ast::nodes::statements::ForNode*>(node))
        {
            auto bodyReturns = collectReturnStatements(forNode->getBody());
            returns.insert(returns.end(), bodyReturns.begin(), bodyReturns.end());
        }
        // Traverse try-catch-finally blocks
        else if (auto* tryNode = dynamic_cast<ast::nodes::statements::TryNode*>(node))
        {
            // Collect returns from try block
            auto tryReturns = collectReturnStatements(tryNode->getTryBlock());
            returns.insert(returns.end(), tryReturns.begin(), tryReturns.end());

            // Collect returns from all catch blocks
            for (const auto& catchBlock : tryNode->getCatchBlocks())
            {
                auto catchReturns = collectReturnStatements(catchBlock->getBody());
                returns.insert(returns.end(), catchReturns.begin(), catchReturns.end());
            }

            // Collect returns from finally block (if present)
            if (tryNode->getFinallyBlock())
            {
                auto finallyReturns = collectReturnStatements(tryNode->getFinallyBlock());
                returns.insert(returns.end(), finallyReturns.begin(), finallyReturns.end());
            }
        }

        return returns;
    }

    void StatementCompiler::validateLambdaAssignment(ast::AssignmentNode* node, bool isReassignment,
                                                     const std::string& existingClassName)
    {
        auto* value = node->getValue();
        if (!value || !dynamic_cast<ast::LambdaNode*>(value))
        {
            return;
        }

        auto* lambdaNode = dynamic_cast<ast::LambdaNode*>(value);
        value::ValueType varType = node->getVariableType();

        // NOTE: Lambda reassignment checking is disabled because we cannot distinguish
        // between first assignment after declaration and lambda-to-lambda reassignment
        // at compile time without runtime value information. Examples:
        //   Function f;      // declaration
        //   f = x -> x * 2;  // first assignment - should be allowed
        //   f = x -> x + 1;  // reassignment - ideally should error, but we can't detect

        // Check if assigning lambda to functional interface
        if (varType == value::ValueType::OBJECT && !node->getClassName().empty())
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

            // Validate lambda matches the interface method signature
            if (interfaceDef && interfaceDef->isFunctionalInterface())
            {
                auto methodSignatures = interfaceDef->getMethodSignatures();
                if (!methodSignatures.empty())
                {
                    const auto& methodSig = methodSignatures[0];

                    // Validate parameter count
                    size_t lambdaParamCount = lambdaNode->getParameters().size();
                    size_t interfaceParamCount = methodSig.parameters.size();
                    if (lambdaParamCount != interfaceParamCount)
                    {
                        throw errors::TypeException(
                            "Lambda parameter count mismatch for interface '" + node->getClassName() + "'. " +
                            "Lambda has " + std::to_string(lambdaParamCount) + " parameter(s) but " +
                            "interface method '" + methodSig.name + "' expects " + std::to_string(interfaceParamCount) + " parameter(s).",
                            node->getLocation()
                        );
                    }

                    // Validate return type - handle both expression and block lambdas
                    // Skip validation for generic type parameters (T, R, etc.) - those are validated at runtime
                    if (methodSig.returnType && !methodSig.returnType->isGenericParameter())
                    {
                        auto* body = lambdaNode->getBody();
                        std::string expectedTypeStr = methodSig.returnType->toString();

                        if (lambdaNode->isExpressionLambda())
                        {
                            // Expression lambda: validate the single expression's type
                            if (body)
                            {
                                value::ValueType lambdaReturnType = ctx.typeInference.inferExpressionType(body);
                                std::string lambdaReturnClassName = ctx.typeInference.inferExpressionClassName(body);

                                // Only validate if we successfully inferred a concrete type (not void)
                                // We skip validation for expressions using parameters (like 'x * 2')
                                // because parameters aren't in scope during this validation phase
                                if (lambdaReturnType != value::ValueType::VOID)
                                {
                                    // Build actual return type string
                                    std::string actualTypeStr;
                                    if (lambdaReturnType == value::ValueType::OBJECT && !lambdaReturnClassName.empty())
                                    {
                                        actualTypeStr = lambdaReturnClassName;
                                    }
                                    else if (lambdaReturnType == value::ValueType::INT)
                                    {
                                        actualTypeStr = "int";
                                    }
                                    else if (lambdaReturnType == value::ValueType::STRING)
                                    {
                                        actualTypeStr = "string";
                                    }
                                    else if (lambdaReturnType == value::ValueType::BOOL)
                                    {
                                        actualTypeStr = "bool";
                                    }
                                    else if (lambdaReturnType == value::ValueType::FLOAT)
                                    {
                                        actualTypeStr = "float";
                                    }
                                    else
                                    {
                                        actualTypeStr = "unknown";
                                    }

                                    // Compare type strings
                                    if (expectedTypeStr != actualTypeStr)
                                    {
                                        throw errors::TypeException(
                                            "Lambda return type mismatch for interface '" + node->getClassName() + "'. " +
                                            "Lambda returns '" + actualTypeStr + "' but " +
                                            "interface method '" + methodSig.name + "' expects '" + expectedTypeStr + "'.",
                                            node->getLocation()
                                        );
                                    }
                                }
                            }
                        }
                        else
                        {
                            // Block lambda: validate all return statements
                            auto returnStatements = collectReturnStatements(body);

                            // Check if lambda expects non-void return but has no return statements
                            if (expectedTypeStr != "void" && returnStatements.empty())
                            {
                                std::string errorMsg = "Lambda is missing return statement. " +
                                    std::string("Interface method '") + methodSig.name + "' expects return type '" + expectedTypeStr + "' " +
                                    "but lambda body has no return statement.";
                                throw errors::TypeException(errorMsg, node->getLocation());
                            }

                            for (auto* returnNode : returnStatements)
                            {
                                auto* returnExpr = returnNode->getReturnValue();
                                if (returnExpr)
                                {
                                    value::ValueType returnType = ctx.typeInference.inferExpressionType(returnExpr);
                                    std::string returnClassName = ctx.typeInference.inferExpressionClassName(returnExpr);

                                    // Build actual return type string
                                    std::string actualTypeStr;
                                    if (returnType == value::ValueType::OBJECT && !returnClassName.empty())
                                    {
                                        actualTypeStr = returnClassName;
                                    }
                                    else if (returnType == value::ValueType::INT)
                                    {
                                        actualTypeStr = "int";
                                    }
                                    else if (returnType == value::ValueType::STRING)
                                    {
                                        actualTypeStr = "string";
                                    }
                                    else if (returnType == value::ValueType::BOOL)
                                    {
                                        actualTypeStr = "bool";
                                    }
                                    else if (returnType == value::ValueType::FLOAT)
                                    {
                                        actualTypeStr = "float";
                                    }
                                    else if (returnType == value::ValueType::VOID)
                                    {
                                        // Skip void returns from parameter references
                                        continue;
                                    }
                                    else
                                    {
                                        actualTypeStr = "unknown";
                                    }

                                    // Compare type strings
                                    if (expectedTypeStr != actualTypeStr)
                                    {
                                        throw errors::TypeException(
                                            "Lambda return type mismatch for interface '" + node->getClassName() + "'. " +
                                            "Lambda returns '" + actualTypeStr + "' but " +
                                            "interface method '" + methodSig.name + "' expects '" + expectedTypeStr + "'.",
                                            returnNode->getLocation()
                                        );
                                    }
                                }
                            }
                        }
                    }
                }
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

        // Check if we're in a real function or just the top-level pseudo-frame
        // Top-level pseudo-frame has scopeDepthStart=0 and localStartSlot=0
        // IMPORTANT: Variables in anonymous blocks (scope depth > 2) should be treated as locals
        // even at global scope, so they can be captured by lambdas
        bool isInRealFunction = false;
        try {
            if (ctx.functionFrameManager.isInFunction()) {
                const auto& frame = ctx.functionFrameManager.currentFrame();

                // If we're in the top-level pseudo-frame (scopeDepthStart=0, localStartSlot=0)
                // but in an anonymous block (scope depth > 3), treat as local for lambda capture
                if (frame.scopeDepthStart == 0 && frame.localStartSlot == 0) {
                    // Check if we're in an anonymous block
                    // Module-level globals are at depth 2-3, anonymous blocks/loops are at depth 4+
                    if (ctx.variableTracker.getCurrentScopeDepth() > 3) {
                        isInRealFunction = true;  // Treat as local for capture semantics
                    } else {
                        isInRealFunction = false;  // Top-level global
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

        int scopeDepth = ctx.variableTracker.getCurrentScopeDepth();
        ctx.globalRegistry.registerGlobal(name, varType, node->getClassName(), scopeDepth);

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

        // PHASE 3: Move reassignment validation to after compilation (for cache to work)
        // Type compatibility validation for reassignments will happen after value compilation

        // Compile the value expression
        if (value)
        {
            // PHASE 3: Compile value FIRST to populate type cache for generic functions
            // PHASE 1: Push expected type context for bidirectional type inference
            // This allows type inference from assignment target (e.g., Box<Int> x = createBox();)
            bool pushedContext = false;
            bool autoBoxed = false;
            bool needsAutoUnbox = false;

            // Determine target class name for auto-boxing check
            std::string targetClassName = isReassignment ? existingClassName : ctx.resolveGenericType(node->getClassName());

            // Try auto-boxing first (for primitive literals assigned to Box types)
            bool shouldAutoBox = false;
            if (!targetClassName.empty())
            {
                if (varType == value::ValueType::OBJECT)
                {
                    // New declaration of Box type
                    shouldAutoBox = true;
                }
                else if (isReassignment)
                {
                    // Reassignment - check if target is a Box type
                    shouldAutoBox = (targetClassName == "Int" || targetClassName == "Float" ||
                                    targetClassName == "Bool" || targetClassName == "String");
                }
            }

            if (shouldAutoBox)
            {
                autoBoxed = tryEmitAutoBoxing(value, targetClassName);
            }

            // Compile value (if not auto-boxed)
            if (!autoBoxed)
            {
                if (varType != value::ValueType::VOID)
                {
                    std::string expectedClassName = isReassignment ? existingClassName : ctx.resolveGenericType(node->getClassName());

                    types::ExpectedTypeContext expectedCtx(varType, expectedClassName);
                    ctx.pushExpectedTypeContext(expectedCtx);
                    pushedContext = true;
                }

                value->accept(ctx.visitor);

                // PHASE 1: Pop expected type context
                if (pushedContext)
                {
                    ctx.popExpectedTypeContext();
                }
            }

            // PHASE 3: NOW do type validation AFTER compilation (so cache is populated)

            // Type compatibility validation for reassignments
            if (isReassignment)
            {
                if (!existingClassName.empty())
                {
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
            }
            // Type validation for declarations
            else if (varType != value::ValueType::VOID)
            {
                value::ValueType valueType = ctx.typeInference.inferExpressionType(value);
                std::string varClassName = ctx.resolveGenericType(node->getClassName());
                std::string valueClassName = ctx.resolveGenericType(ctx.typeInference.inferExpressionClassName(value));
                bool isNullValue = dynamic_cast<ast::NullNode*>(value) != nullptr;

                // PHASE 4: Skip validation if we auto-boxed or can auto-box
                bool canAutoBox = autoBoxed;
                if (!canAutoBox && varType == value::ValueType::OBJECT &&
                    ((varClassName == "Int" && valueType == value::ValueType::INT) ||
                     (varClassName == "Float" && valueType == value::ValueType::FLOAT) ||
                     (varClassName == "Bool" && valueType == value::ValueType::BOOL) ||
                     (varClassName == "String" && valueType == value::ValueType::STRING)))
                {
                    canAutoBox = true;
                }

                if (!canAutoBox)
                {
                    ctx.typeValidator.validateAssignment(varType, varClassName, valueType,
                                                         valueClassName, isNullValue, node->getLocation());
                }

                // Check for auto-unboxing (Box type to primitive)
                if (!autoBoxed && varType != value::ValueType::OBJECT)
                {
                    // Variable is primitive, check if value is a Box object
                    if (valueType == value::ValueType::OBJECT)
                    {
                        if ((varType == value::ValueType::INT && valueClassName == "Int") ||
                            (varType == value::ValueType::FLOAT && valueClassName == "Float") ||
                            (varType == value::ValueType::BOOL && valueClassName == "Bool") ||
                            (varType == value::ValueType::STRING && valueClassName == "String"))
                        {
                            needsAutoUnbox = true;
                            // Emit auto-unbox call
                            size_t methodNameIndex = ctx.program.getConstantPool().addString("getValue");
                            ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                                         static_cast<uint32_t>(methodNameIndex),
                                                         0u,  // 0 arguments
                                                         value);
                        }
                    }
                }
            }
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
        // However, anonymous blocks SHOULD create their own scopes, even inside functions.

        // Determine if we should manage scope for this block:
        // - Always manage scope if not in a function
        // - If in the global pseudo-frame (scopeDepthStart=0, localStartSlot=0), manage scope for anonymous blocks
        // - For real function bodies, the scope is managed by the function compiler
        bool shouldManageScope = false;

        if (!ctx.functionFrameManager.isInFunction())
        {
            shouldManageScope = true;
        }
        else
        {
            const auto& frame = ctx.functionFrameManager.currentFrame();
            // If we're in the global pseudo-frame, anonymous blocks should manage their own scopes
            if (frame.scopeDepthStart == 0 && frame.localStartSlot == 0)
            {
                shouldManageScope = true;
            }
        }

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

        // Only add a scope if we're compiling the MAIN file (at scope 2)
        // Imported files are already at scope 3, so they shouldn't add another scope
        bool shouldManageScope = (ctx.variableTracker.getCurrentScopeDepth() == 2);

        if (shouldManageScope) {
            // Create scope 3 for module-level globals (both main file and imported files)
            ctx.variableTracker.beginScope();
        }

        const auto& statements = node->getStatements();
        for (auto& stmt : statements)
        {
            stmt->accept(ctx.visitor); // Will need delegation
        }

        if (shouldManageScope) {
            ctx.variableTracker.endScope();
        }

        // Clean up any variables that went out of scope (e.g., from nested blocks/loops at scope 4+)
        ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

        return std::monostate{};
    }

    // Phase 4: Auto-boxing helper for assignments
    bool StatementCompiler::tryEmitAutoBoxing(ast::ASTNode* valueNode, const std::string& targetClassName)
    {
        using namespace ast::nodes::expressions;
        using namespace ast::nodes::classes;

        // Only auto-box for primitive Box types
        bool isBoxType = (targetClassName == "Int" ||
                          targetClassName == "Float" ||
                          targetClassName == "Bool" ||
                          targetClassName == "String");

        if (!isBoxType || !valueNode)
        {
            return false;  // Not a Box type or no value node
        }

        // PHASE 4: Check if value expression returns a primitive type matching the target Box type
        value::ValueType valueType = ctx.typeInference.inferExpressionType(valueNode);
        bool needsBoxing = false;

        // Check if we're trying to box a primitive to its corresponding Box type
        if (targetClassName == "Int" && valueType == value::ValueType::INT)
        {
            needsBoxing = true;
        }
        else if (targetClassName == "Float" && valueType == value::ValueType::FLOAT)
        {
            needsBoxing = true;
        }
        else if (targetClassName == "Bool" && valueType == value::ValueType::BOOL)
        {
            needsBoxing = true;
        }
        else if (targetClassName == "String" && valueType == value::ValueType::STRING)
        {
            needsBoxing = true;
        }

        if (!needsBoxing)
        {
            return false;  // Value type doesn't match target Box type
        }

        // PHASE 4 AUTO-BOXING: Emit bytecode for boxing
        // Equivalent to: new TargetClass(value)

        // 1. Compile the value expression (pushes it onto stack)
        valueNode->accept(ctx.visitor);

        // 2. Emit NEW_OBJECT for the Box class
        size_t classNameIndex = ctx.program.getConstantPool().addString(targetClassName);
        ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                     static_cast<uint32_t>(classNameIndex),
                                     1u,  // 1 constructor argument
                                     valueNode);

        return true;  // Auto-boxing was applied
    }
}
