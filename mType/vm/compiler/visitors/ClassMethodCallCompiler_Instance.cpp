#include "ClassCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "../validation/CompileTimeValidator.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../optimization/PrimitiveMethodOptimizer.hpp"
#include "../../../errors/TypeException.hpp"

namespace vm::compiler::visitors
{
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
            std::string qualifiedMethodName = baseClassName.empty() ? methodName : (baseClassName + "::" + methodName);
            const auto* methodMetadata = ctx.program.getFunction(resolvedMethodName);

            // Fallback to class definition if bytecode metadata not available
            std::shared_ptr<runtimeTypes::klass::MethodDefinition> methodDef;
            if (!methodMetadata && !baseClassName.empty())
            {
                auto classDef = ctx.env->findClass(baseClassName);
                if (classDef)
                {
                    auto overloads = classDef->getAllInstanceMethodOverloads(methodName);
                    if (overloads.size() == 1)
                    {
                        methodDef = overloads[0];
                    }
                    else if (overloads.size() > 1)
                    {
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

                // Skip validation if we couldn't determine method info (e.g., chained calls in Phase 1)
                if (!isGenericMethod && (methodMetadata || methodDef))
                {
                    throw errors::TypeException(
                        "Method '" + qualifiedMethodName + "' is not generic but used with type arguments",
                        node->getLocation()
                    );
                }

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

                for (size_t i = 0; i < methodGenericParams.size() && i < methodTypeArgs.size(); ++i)
                {
                    methodGenericBindings[methodGenericParams[i]] = methodTypeArgs[i];
                }

                if (!methodGenericBindings.empty())
                {
                    ctx.pushGenericTypeBindings(methodGenericBindings);
                    hasMethodGenericBindings = true;
                }
            }
            else
            {
                // PHASE 1: Require explicit type arguments for generic methods
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

                std::string expectedType;
                bool hasExpectedType = false;

                if (methodMetadata && !methodMetadata->parameterTypes.empty())
                {
                    // For instance methods, parameterTypes[0] is 'this', so offset by 1
                    size_t paramIndex = i + 1;

                    if (paramIndex < methodMetadata->parameterTypes.size())
                    {
                        expectedType = methodMetadata->parameterTypes[paramIndex];
                        hasExpectedType = true;
                    }
                }
                else if (methodDef && !methodDef->getUnifiedParameters().empty())
                {
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
                    expectedType = interfaceParameterTypes[i];
                    hasExpectedType = true;
                }

                if (hasExpectedType)
                {
                    // Check method-level bindings first, then class-level bindings
                    if (!methodGenericBindings.empty())
                    {
                        auto it = methodGenericBindings.find(expectedType);
                        if (it != methodGenericBindings.end())
                        {
                            expectedType = it->second;
                        }
                    }
                    // Resolve nested generic parameters too, e.g.
                    // Predicate<T> on Stream<Int> -> Predicate<Int>.
                    expectedType = ctx.resolveGenericType(expectedType);

                    value::ValueType argType = ctx.typeInference.inferExpressionType(arguments[i].get());

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

                if (!autoBoxed)
                {
                    bool pushedExpectedType = false;
                    if (hasExpectedType)
                    {
                        value::ValueType expectedValueType = ::types::TypeConversionUtils::stringToValueType(expectedType);
                        if (expectedValueType == value::ValueType::OBJECT && !expectedType.empty() &&
                            !::types::TypeConversionUtils::containsGenericTypeParameter(expectedType))
                        {
                            ctx.pushExpectedTypeContext(types::ExpectedTypeContext(expectedValueType, expectedType));
                            pushedExpectedType = true;
                        }
                    }

                    arguments[i]->accept(ctx.visitor);

                    if (pushedExpectedType)
                    {
                        ctx.popExpectedTypeContext();
                    }
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

            // MYT-228: stage method-level type-arg bindings into the next frame.
            // Class-level bindings (on the receiver via ObjectInstance::genericTypeBindings)
            // don't need forwarding — resolveTypeParameter walks them via getThisInstanceRaw.
            // Skip BIND_TYPE_ARGS for primitive-optimized opcodes — those don't push a
            // CallFrame so there'd be nothing to consume the staged bindings.
            bool isPrimitiveOptimized =
                vm::compiler::PrimitiveMethodOptimizer::canOptimizeMethod(baseClassName, methodName, arguments.size());
            if (!isPrimitiveOptimized && !methodGenericBindings.empty())
            {
                emitBindTypeArgsForMethodCall(node, methodGenericBindings);
            }

            // PHASE 3 OPTIMIZATION: optimizable primitive method call?
            bytecode::OpCode opcodeToEmit = bytecode::OpCode::CALL_METHOD;

            if (isPrimitiveOptimized) {
                opcodeToEmit = vm::compiler::PrimitiveMethodOptimizer::getOptimizedOpCode(baseClassName, methodName);
                ctx.emitter.emitWithLocation(opcodeToEmit, node);
            } else {
                size_t methodNameIndex = ctx.program.getConstantPool().addString(resolvedMethodName);
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                 static_cast<uint64_t>(methodNameIndex),
                                 static_cast<uint64_t>(arguments.size()), node);
                ctx.program.setLastInstructionFlags(bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER);
            }
        }
    }
}
