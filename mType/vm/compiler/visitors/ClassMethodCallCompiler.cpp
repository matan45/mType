#include "ClassCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "GenericScopeHelper.hpp"
#include "../validation/CompileTimeValidator.hpp"
#include "../../../runtimeTypes/klass/SignatureUtils.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../optimization/PrimitiveMethodOptimizer.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../ast/nodes/functions/FunctionNode.hpp"
#include "../../../types/TypeSubstitutionService.hpp"
#include <iostream>

namespace vm::compiler::visitors
{
    void ClassCompiler::emitBindTypeArgsForMethodCall(
        ast::ASTNode* node,
        const std::unordered_map<std::string, std::string>& bindings)
    {
        // MYT-228: stage method-level type-parameter bindings into
        // ExecutionContext::pendingTypeArgs. The next pushCallFrame
        // (triggered by the terminal CALL_*) consumes them into the new
        // frame's typeArgBindings. Forward-from-caller detection lives in
        // isTypeParamInScope (GenericScopeHelper.hpp) — shared with
        // FunctionCallHelper and ExpressionCompiler.
        if (bindings.empty())
        {
            return;
        }

        std::vector<uint64_t> operands;
        operands.reserve(1 + 3 * bindings.size());
        operands.push_back(static_cast<uint64_t>(bindings.size()));

        for (const auto& [paramName, value] : bindings)
        {
            const auto kind = isTypeParamInScope(ctx, value)
                ? bytecode::TypeArgValueKind::ForwardFromCaller
                : bytecode::TypeArgValueKind::Concrete;
            size_t paramNameIdx = ctx.program.getConstantPool().addString(paramName);
            size_t valueIdx = ctx.program.getConstantPool().addString(value);

            operands.push_back(static_cast<uint64_t>(paramNameIdx));
            operands.push_back(static_cast<uint64_t>(kind));
            operands.push_back(static_cast<uint64_t>(valueIdx));
        }

        ctx.emitter.emitWithLocation(bytecode::OpCode::BIND_TYPE_ARGS, operands, node);
    }

    void ClassCompiler::compileStaticMethodCall(ast::MethodCallNode* node)
    {
        std::string methodName = node->getMethodName();
        const auto& arguments = node->getArguments();
        {
            // Static method call: ClassName::methodName(args) or this::methodName(args)

            // Build fully qualified method name
            std::string className;
            auto* objectNode = node->getObject();
            if (auto* varNode = dynamic_cast<ast::VariableNode*>(objectNode))
            {
                className = varNode->getName();
                // If using "this::", replace with actual class name
                if (className == "this" && ctx.currentClassNode)
                {
                    className = ctx.currentClassNode->getClassName();
                }
            }
            std::string qualifiedName = className + "::" + methodName;

            // Resolve method overload to get mangled name. Forward explicit generic
            // type arguments (MYT-224) so the resolver can substitute them into
            // declared parameter types before the unification check.
            std::string resolvedMethodName = overloadResolver->resolveStaticMethodOverload(
                className, methodName, arguments, node->getLocation(),
                node->hasGenericTypeArguments(),
                node->getGenericTypeArguments());

            // PHASE 4: Validate generic type arguments
            if (node->hasGenericTypeArguments())
            {
                const auto& typeArgs = node->getGenericTypeArguments();

                // Validate: Generic type arguments must be object types, not primitives
                for (const auto& typeArg : typeArgs)
                {
                    if (typeArg == "int" || typeArg == "float" || typeArg == "string" ||
                        typeArg == "bool" || typeArg == "void")
                    {
                        throw errors::TypeException(
                            "Generic type argument '" + typeArg + "' is a primitive type. " +
                            "Generics only support object types. Use wrapper classes instead:\n" +
                            "  - Use 'Int' instead of 'int'\n" +
                            "  - Use 'Float' instead of 'float'\n" +
                            "  - Use 'Bool' instead of 'bool'\n" +
                            "  - Use 'String' instead of 'string'",
                            node->getLocation()
                        );
                    }
                }

                // Validate: Method must be generic
                const auto* funcMetadata = ctx.program.getFunction(qualifiedName);
                if (funcMetadata) {
                    const auto& genericParams = funcMetadata->genericTypeParameters;

                    // Validate: Non-generic method cannot be used with type arguments
                    if (genericParams.empty()) {
                        throw errors::TypeException(
                            "Method '" + qualifiedName + "' is not generic but used with type arguments",
                            node->getLocation()
                        );
                    }

                    // Validate: Number of type arguments must match number of generic parameters
                    if (typeArgs.size() != genericParams.size()) {
                        throw errors::TypeException(
                            "Method '" + qualifiedName + "' expects " +
                            std::to_string(genericParams.size()) +
                            " type argument(s) but " + std::to_string(typeArgs.size()) + " provided",
                            node->getLocation()
                        );
                    }
                }
                else {
                    // Fallback: Check environment if metadata not yet registered
                    auto classDef = ctx.env->findClass(className);
                    if (classDef) {
                        auto methodDef = classDef->getMethod(methodName);
                        if (methodDef) {
                            const auto& genericParams = methodDef->getGenericTypeParameters();

                            // Validate: Non-generic method cannot be used with type arguments
                            if (genericParams.empty()) {
                                throw errors::TypeException(
                                    "Method '" + qualifiedName + "' is not generic but used with type arguments",
                                    node->getLocation()
                                );
                            }

                            // Validate: Number of type arguments must match number of generic parameters
                            if (typeArgs.size() != genericParams.size()) {
                                throw errors::TypeException(
                                    "Method '" + qualifiedName + "' expects " +
                                    std::to_string(genericParams.size()) +
                                    " type argument(s) but " + std::to_string(typeArgs.size()) + " provided",
                                    node->getLocation()
                                );
                            }
                        }
                    }
                }
            }

            // Validate static method exists at compile time
            if (ctx.compileTimeValidator)
            {
                ctx.compileTimeValidator->validateStaticMethodExists(className, methodName, arguments.size(), node->getLocation());
            }

            // Validate parameter count and types
            paramValidator->validateMethodParameters(qualifiedName, qualifiedName, arguments, node->getLocation());

            // Get method metadata for auto-boxing
            const auto* methodMetadata = ctx.program.getFunction(qualifiedName);

            // Push all arguments onto stack (with auto-boxing if needed)
            for (size_t i = 0; i < arguments.size(); ++i)
            {
                bool autoBoxed = false;

                // Try auto-boxing if we have method metadata
                if (methodMetadata && !methodMetadata->isNative)
                {
                    // For instance methods, parameterTypes[0] is 'this', so offset by 1
                    size_t paramOffset = (!methodMetadata->isStatic && !methodMetadata->parameterTypes.empty()) ? 1 : 0;

                    if (i + paramOffset < methodMetadata->parameterTypes.size())
                    {
                        std::string expectedType = methodMetadata->parameterTypes[i + paramOffset];
                        expectedType = ctx.resolveGenericType(expectedType);
                        autoBoxed = tryAutoBoxArgument(arguments[i].get(), expectedType);
                    }
                }

                // If not auto-boxed, compile argument normally
                if (!autoBoxed)
                {
                    arguments[i]->accept(ctx.visitor);
                }
            }

            // MYT-228: stage method-level type-arg bindings into the next
            // frame so `obj isClassOf T` and `(T)cast` inside the callee
            // can resolve T against the call's type arguments.
            if (node->hasGenericTypeArguments())
            {
                std::unordered_map<std::string, std::string> bindings;
                const auto& typeArgs = node->getGenericTypeArguments();

                std::vector<std::string> genericParamNames;
                if (methodMetadata && !methodMetadata->genericTypeParameters.empty())
                {
                    genericParamNames = methodMetadata->genericTypeParameters;
                }
                else
                {
                    auto classDef = ctx.env->findClass(className);
                    if (classDef)
                    {
                        auto methodDef = classDef->getMethod(methodName);
                        if (methodDef)
                        {
                            for (const auto& p : methodDef->getGenericTypeParameters())
                            {
                                genericParamNames.push_back(p.name);
                            }
                        }
                    }
                }

                for (size_t i = 0; i < genericParamNames.size() && i < typeArgs.size(); ++i)
                {
                    bindings[genericParamNames[i]] = typeArgs[i];
                }
                emitBindTypeArgsForMethodCall(node, bindings);
            }

            // Emit CALL_STATIC instruction with resolved (mangled) method name.
            // MYT-197: $static suffix must live in the constant pool — the
            // runtime CALL_STATIC handler no longer appends it. Idempotent
            // helper covers both happy-path (already suffixed by overload
            // resolution) and bypass paths in one call.
            runtimeTypes::klass::SignatureUtils::ensureStaticSuffix(resolvedMethodName);
            size_t methodNameIndex = ctx.program.getConstantPool().addString(resolvedMethodName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_STATIC,
                             static_cast<uint64_t>(methodNameIndex),
                             static_cast<uint64_t>(arguments.size()), node);
        }
    }

    void ClassCompiler::compileInstanceMethodCall(ast::MethodCallNode* node)
    {
        std::string methodName = node->getMethodName();
        const auto& arguments = node->getArguments();
        {
            // Instance method call: object.methodName(args)

            // Infer the class of the object for type checking
            std::string objectClassName = ctx.typeInference.inferExpressionClassName(node->getObject());

            // Resolve method overload to get mangled name (if class known)
            std::string resolvedMethodName = methodName;
            if (!objectClassName.empty())
            {
                resolvedMethodName = overloadResolver->resolveInstanceMethodOverload(
                    objectClassName, methodName, arguments, node->getLocation(),
                    node->hasGenericTypeArguments(), node->getGenericTypeArguments().size());
            }

            // Extract generic type bindings from objectClassName if it's a generic instantiation
            // E.g., "Box<String>" -> push {T: String} onto the binding stack
            std::unordered_map<std::string, std::string> genericBindings;
            if (objectClassName.find('<') != std::string::npos)
            {
                // Parse "Box<String>" into base class and type arguments
                size_t angleStart = objectClassName.find('<');
                size_t angleEnd = objectClassName.rfind('>');
                if (angleEnd != std::string::npos && angleEnd > angleStart)
                {
                    std::string baseClassName = objectClassName.substr(0, angleStart);
                    std::string typeArgsStr = objectClassName.substr(angleStart + 1, angleEnd - angleStart - 1);

                    // Look up the base class OR interface to get its generic parameter names
                    auto classDef = ctx.env->findClass(baseClassName);
                    auto interfaceDef = ctx.env->findInterface(baseClassName);

                    std::vector<ast::GenericTypeParameter> genericParams;
                    bool isGenericType = false;

                    if (classDef && classDef->isGeneric())
                    {
                        genericParams = classDef->getGenericParameters();
                        isGenericType = true;
                    }
                    else if (interfaceDef && interfaceDef->isGeneric())
                    {
                        genericParams = interfaceDef->getGenericParameters();
                        isGenericType = true;
                    }

                    if (isGenericType)
                    {

                        // Parse type arguments (e.g., "String" or "String, Int")
                        std::vector<std::string> typeArgs;
                        size_t start = 0;
                        size_t depth = 0;
                        for (size_t i = 0; i < typeArgsStr.length(); ++i)
                        {
                            if (typeArgsStr[i] == '<') depth++;
                            else if (typeArgsStr[i] == '>') depth--;
                            else if (typeArgsStr[i] == ',' && depth == 0)
                            {
                                std::string arg = typeArgsStr.substr(start, i - start);
                                arg.erase(0, arg.find_first_not_of(" \t"));
                                arg.erase(arg.find_last_not_of(" \t") + 1);
                                typeArgs.push_back(arg);
                                start = i + 1;
                            }
                        }
                        // Add last argument
                        std::string arg = typeArgsStr.substr(start);
                        arg.erase(0, arg.find_first_not_of(" \t"));
                        arg.erase(arg.find_last_not_of(" \t") + 1);
                        if (!arg.empty())
                        {
                            typeArgs.push_back(arg);
                        }

                        // Create bindings: T -> String, etc.
                        for (size_t i = 0; i < genericParams.size() && i < typeArgs.size(); ++i)
                        {
                            genericBindings[genericParams[i].name] = typeArgs[i];
                        }
                    }
                }
            }

            // Push generic type bindings onto the stack for resolution during validation
            if (!genericBindings.empty())
            {
                ctx.pushGenericTypeBindings(genericBindings);
            }

            // Setup method-level generic type bindings if method has type arguments
            bool hasMethodGenericBindings = false;
            std::unordered_map<std::string, std::string> methodGenericBindings;

            // Extract base class name (without nullable suffix or generic parameters) for method lookup
            std::string baseClassName = ::types::TypeConversionUtils::stripNullable(objectClassName);
            size_t anglePos = baseClassName.find('<');
            if (anglePos != std::string::npos)
            {
                baseClassName = baseClassName.substr(0, anglePos);
            }

            // Build qualified method name for metadata lookup
            // Use resolvedMethodName (with signature) for correct overload identification
            std::string qualifiedMethodName = baseClassName.empty() ? methodName : (baseClassName + "::" + methodName);
            const auto* methodMetadata = ctx.program.getFunction(resolvedMethodName);

            // Fallback to class definition if bytecode metadata not available
            std::shared_ptr<runtimeTypes::klass::MethodDefinition> methodDef;
            if (!methodMetadata && !baseClassName.empty())
            {
                auto classDef = ctx.env->findClass(baseClassName);
                if (classDef)
                {
                    // Try to find the method by name (may have overloads)
                    auto overloads = classDef->getAllInstanceMethodOverloads(methodName);
                    if (overloads.size() == 1)
                    {
                        methodDef = overloads[0];
                    }
                    else if (overloads.size() > 1)
                    {
                        // Multiple overloads - find one that matches the argument count
                        // (this is for validation purposes; overload resolution happens elsewhere)
                        size_t argCount = arguments.size();
                        for (const auto& overload : overloads)
                        {
                            // Instance methods have 'this' as first param, so compare argCount+1
                            if (overload->getParameters().size() == argCount + 1)
                            {
                                methodDef = overload;
                                break;
                            }
                        }
                        // If no exact match, just use the first one for generic validation
                        if (!methodDef && !overloads.empty())
                        {
                            methodDef = overloads[0];
                        }
                    }
                }
            }

            if (node->hasGenericTypeArguments())
            {
                const auto& methodTypeArgs = node->getGenericTypeArguments();

                // PHASE 4: Validate that method is generic
                bool isGenericMethod = false;
                size_t expectedTypeParamCount = 0;

                if (methodMetadata)
                {
                    isGenericMethod = !methodMetadata->genericTypeParameters.empty();
                    expectedTypeParamCount = methodMetadata->genericTypeParameters.size();
                }
                else if (methodDef)
                {
                    isGenericMethod = !methodDef->getGenericTypeParameters().empty();
                    expectedTypeParamCount = methodDef->getGenericTypeParameters().size();
                }

                // Validate: Non-generic method cannot have type arguments
                // Skip validation if we couldn't determine method info (e.g., chained calls in Phase 1)
                if (!isGenericMethod && (methodMetadata || methodDef))
                {
                    throw errors::TypeException(
                        "Method '" + qualifiedMethodName + "' is not generic but used with type arguments",
                        node->getLocation()
                    );
                }

                // Validate: Type argument count must match
                // Only validate if we have method information
                if ((methodMetadata || methodDef) && methodTypeArgs.size() != expectedTypeParamCount)
                {
                    throw errors::TypeException(
                        "Method '" + methodName + "' expects " +
                        std::to_string(expectedTypeParamCount) +
                        " type argument(s), but got " +
                        std::to_string(methodTypeArgs.size()),
                        node->getLocation()
                    );
                }

                // Validate: Generic type arguments must be object types, not primitives
                for (const auto& typeArg : methodTypeArgs)
                {
                    if (typeArg == "int" || typeArg == "float" || typeArg == "string" ||
                        typeArg == "bool" || typeArg == "void")
                    {
                        throw errors::TypeException(
                            "Generic type argument '" + typeArg + "' is a primitive type. " +
                            "Generics only support object types. Use wrapper classes instead:\n" +
                            "  - Use 'Int' instead of 'int'\n" +
                            "  - Use 'Float' instead of 'float'\n" +
                            "  - Use 'Bool' instead of 'bool'\n" +
                            "  - Use 'String' instead of 'string'",
                            node->getLocation()
                        );
                    }
                }

                // Create bindings from type arguments
                std::vector<std::string> methodGenericParams;
                if (methodMetadata)
                {
                    methodGenericParams = methodMetadata->genericTypeParameters;
                }
                else if (methodDef)
                {
                    for (const auto& param : methodDef->getGenericTypeParameters())
                    {
                        methodGenericParams.push_back(param.name);
                    }
                }

                // Create bindings: T -> String, U -> Int, etc.
                for (size_t i = 0; i < methodGenericParams.size() && i < methodTypeArgs.size(); ++i)
                {
                    methodGenericBindings[methodGenericParams[i]] = methodTypeArgs[i];
                }

                // Push method-level bindings onto stack (will shadow class-level if names conflict)
                if (!methodGenericBindings.empty())
                {
                    ctx.pushGenericTypeBindings(methodGenericBindings);
                    hasMethodGenericBindings = true;
                }
            }
            else
            {
                // PHASE 1: Require explicit type arguments for generic methods
                // Check if method is generic - if so, error that type args are required
                bool requiresTypeArgs = false;
                if (methodMetadata && !methodMetadata->genericTypeParameters.empty())
                {
                    requiresTypeArgs = true;
                }
                else if (methodDef && !methodDef->getGenericTypeParameters().empty())
                {
                    requiresTypeArgs = true;
                }

                if (requiresTypeArgs)
                {
                    throw errors::TypeException(
                        "Generic method '" + methodName + "' requires explicit type arguments. " +
                        "Phase 1 does not support type inference for method calls.",
                        node->getLocation()
                    );
                }
            }

            // Validate instance method exists at compile time
            if (!objectClassName.empty() && ctx.compileTimeValidator)
            {
                // Extract base class name (without nullable suffix or generic parameters)
                std::string baseClassName = ::types::TypeConversionUtils::stripNullable(objectClassName);
                size_t anglePos = baseClassName.find('<');
                if (anglePos != std::string::npos)
                {
                    baseClassName = baseClassName.substr(0, anglePos);
                }
                ctx.compileTimeValidator->validateInstanceMethodExists(baseClassName, methodName, arguments.size(), node->getLocation());
            }

            // Validate parameter count and types for instance methods
            if (!objectClassName.empty())
            {
                // Use the resolved method name (with signature) for validation after overload resolution
                paramValidator->validateMethodParameters(methodName, resolvedMethodName, arguments, node->getLocation());
            }

            // First, compile the object expression
            bool nonNullReceiver = isReceiverNonNullable(node->getObject());

            if (!nonNullReceiver)
            {
                throw errors::TypeException(
                    "Cannot call method '" + methodName + "' on nullable receiver. "
                    "Add a null check (if (x != null) { ... }) or change the receiver's declared type to non-nullable.",
                    node->getLocation()
                );
            }

            node->getObject()->accept(ctx.visitor);

            // Push all arguments onto stack with auto-boxing if needed
            // If no methodMetadata, try looking up interface definition
            std::vector<std::string> interfaceParameterTypes;
            if (!methodMetadata && !baseClassName.empty())
            {
                auto interfaceDef = ctx.env->findInterface(baseClassName);
                if (interfaceDef)
                {
                    const auto& methodSigs = interfaceDef->getMethodSignatures();
                    for (const auto& methodSig : methodSigs)
                    {
                        if (methodSig.name == methodName)
                        {
                            // Extract parameter type names from GenericType objects
                            for (const auto& param : methodSig.parameters)
                            {
                                std::string paramTypeName = param.second->toString();
                                interfaceParameterTypes.push_back(paramTypeName);
                            }
                            break;
                        }
                    }
                }
            }

            for (size_t i = 0; i < arguments.size(); ++i)
            {
                bool autoBoxed = false;

                // Check if we need to auto-box this argument
                std::string expectedType;
                bool hasExpectedType = false;

                if (methodMetadata && !methodMetadata->parameterTypes.empty())
                {
                    // For instance methods, parameterTypes[0] is 'this', so offset by 1
                    size_t paramIndex = i + 1; // +1 to skip 'this' parameter

                    if (paramIndex < methodMetadata->parameterTypes.size())
                    {
                        expectedType = methodMetadata->parameterTypes[paramIndex];
                        hasExpectedType = true;
                    }
                }
                else if (methodDef && !methodDef->getUnifiedParameters().empty())
                {
                    // Fallback to method definition from ClassDefinition
                    const auto& uParams = methodDef->getUnifiedParameters();
                    // unifiedParameters already exclude 'this' for instance methods
                    if (i < uParams.size())
                    {
                        expectedType = uParams[i].second ? uParams[i].second->toString() : "void";
                        hasExpectedType = true;
                    }
                }
                else if (!interfaceParameterTypes.empty() && i < interfaceParameterTypes.size())
                {
                    // Use interface parameter types
                    expectedType = interfaceParameterTypes[i];
                    hasExpectedType = true;
                }

                if (hasExpectedType)
                {
                    // Resolve generic type parameters using the bindings
                    // Check method-level bindings first, then class-level bindings
                    // E.g., if expectedType is "A" and methodGenericBindings has {A: String}, resolve to "String"
                    if (!methodGenericBindings.empty())
                    {
                        auto it = methodGenericBindings.find(expectedType);
                        if (it != methodGenericBindings.end())
                        {
                            expectedType = it->second;
                        }
                    }
                    // If not found in method bindings, check class-level bindings
                    // E.g., if expectedType is "T" and genericBindings has {T: Int}, resolve to "Int"
                    if (!genericBindings.empty())
                    {
                        auto it = genericBindings.find(expectedType);
                        if (it != genericBindings.end())
                        {
                            expectedType = it->second;
                        }
                    }

                    value::ValueType argType = ctx.typeInference.inferExpressionType(arguments[i].get());

                        // Check if we need to auto-box primitive to wrapper class
                        bool needsBoxing = false;
                        std::string boxClassName;

                        if (expectedType == "Int" && argType == value::ValueType::INT)
                        {
                            needsBoxing = true;
                            boxClassName = "Int";
                        }
                        else if (expectedType == "Float" && argType == value::ValueType::FLOAT)
                        {
                            needsBoxing = true;
                            boxClassName = "Float";
                        }
                        else if (expectedType == "Bool" && argType == value::ValueType::BOOL)
                        {
                            needsBoxing = true;
                            boxClassName = "Bool";
                        }
                        else if (expectedType == "String" && argType == value::ValueType::STRING)
                        {
                            needsBoxing = true;
                            boxClassName = "String";
                        }

                        if (needsBoxing)
                        {
                            // Auto-box: compile value, then emit NEW_OBJECT or NEW_VALUE_OBJECT
                            arguments[i]->accept(ctx.visitor);
                            size_t classNameIndex = ctx.program.getConstantPool().addString(boxClassName);
                            auto boxClassDef = ctx.env->findClass(boxClassName);
                            bool boxIsValue = boxClassDef && boxClassDef->isValueClass();
                            if (boxIsValue) {
                                ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_VALUE_OBJECT,
                                                            static_cast<uint64_t>(classNameIndex),
                                                            1u, arguments[i].get());
                                ctx.emitter.emitWithLocation(bytecode::OpCode::OBJECT_TO_VALUE, arguments[i].get());
                            } else {
                                ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                                            static_cast<uint64_t>(classNameIndex),
                                                            1u, arguments[i].get());
                            }
                            autoBoxed = true;
                        }
                }

                // If not auto-boxed, compile argument normally
                if (!autoBoxed)
                {
                    arguments[i]->accept(ctx.visitor);
                }
            }

            // Pop generic type bindings after argument compilation (method-level first, then class-level)
            if (hasMethodGenericBindings)
            {
                ctx.popGenericTypeBindings();
            }
            if (!genericBindings.empty())
            {
                ctx.popGenericTypeBindings();
            }

            // MYT-228: stage method-level type-arg bindings into the next
            // frame. Class-level bindings (already on the receiver via
            // ObjectInstance::genericTypeBindings) don't need forwarding —
            // resolveTypeParameter walks them via getThisInstanceRaw.
            // Skip the BIND_TYPE_ARGS path for primitive-optimized opcodes
            // — those don't push a CallFrame so there'd be nothing to
            // consume the staged bindings.
            bool isPrimitiveOptimized =
                vm::compiler::PrimitiveMethodOptimizer::canOptimizeMethod(baseClassName, methodName, arguments.size());
            if (!isPrimitiveOptimized && !methodGenericBindings.empty())
            {
                emitBindTypeArgsForMethodCall(node, methodGenericBindings);
            }

            // PHASE 3 OPTIMIZATION: Check if this is an optimizable primitive method call
            bytecode::OpCode opcodeToEmit = bytecode::OpCode::CALL_METHOD;

            if (isPrimitiveOptimized) {
                // Get the optimized opcode for this primitive method
                opcodeToEmit = vm::compiler::PrimitiveMethodOptimizer::getOptimizedOpCode(baseClassName, methodName);

                // Emit optimized opcode (no method name needed - opcode encodes the operation)
                ctx.emitter.emitWithLocation(opcodeToEmit, node);
            } else {
                // Emit standard CALL_METHOD instruction with resolved (mangled) method name
                size_t methodNameIndex = ctx.program.getConstantPool().addString(resolvedMethodName);
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                 static_cast<uint64_t>(methodNameIndex),
                                 static_cast<uint64_t>(arguments.size()), node);
                ctx.program.setLastInstructionFlags(bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER);
            }
        }
    }

}

