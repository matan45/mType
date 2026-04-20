#include "FunctionCallHelper.hpp"
#include "../validation/CompileTimeValidator.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../errors/EnvironmentException.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../types/GenericPatternAnalyzer.hpp"
#include <optional>
#include <iostream>
namespace vm::compiler::visitors
{
    FunctionCallHelper::FunctionCallHelper(CompilerContext& context)
        : ctx(context)
        , overloadResolver(std::make_unique<overload::OverloadResolutionHelper>(context))
    {
    }

    std::string FunctionCallHelper::inferTypeFromArgument(ast::ASTNode* argument)
    {
        // Infer the type of an argument for generic type inference
        value::ValueType argType = ctx.typeInference.inferExpressionType(argument);

        if (argType == value::ValueType::VOID)
        {
            return ""; // Cannot infer - unknown type
        }

        // For primitive types, return the type name
        if (argType != value::ValueType::OBJECT)
        {
            return ::types::TypeConversionUtils::getTypeDisplayName(argType);
        }

        // For object types, return the class name
        std::string className = ctx.typeInference.inferExpressionClassName(argument);
        return className;
    }

    bool FunctionCallHelper::setupGenericTypeBindings(ast::FunctionCallNode* node, const std::string& functionName)
    {
        const auto* funcMetadata = ctx.program.getFunction(functionName);
        if (!funcMetadata)
        {
            // Function not yet registered - this can happen for forward references
            return false;
        }

        // If not a generic function, nothing to do
        if (funcMetadata->genericTypeParameters.empty())
        {
            // But if user provided type arguments, that's an error
            if (node->hasGenericTypeArguments())
            {
                throw errors::TypeException(
                    "Function '" + functionName + "' is not generic but generic type arguments were provided",
                    node->getLocation());
            }
            return false;
        }

        std::unordered_map<std::string, std::string> typeBindings;

        // PHASE 3: Three-tier inference strategy
        // Tier 1: Explicit type arguments (highest priority)
        if (node->hasGenericTypeArguments())
        {
            const auto& genericTypeArgs = node->getGenericTypeArguments();
            const auto& genericTypeParams = funcMetadata->genericTypeParameters;

            if (genericTypeArgs.size() != genericTypeParams.size())
            {
                throw errors::TypeException(
                    "Function '" + functionName + "' expects " +
                    std::to_string(genericTypeParams.size()) + " type argument(s) but got " +
                    std::to_string(genericTypeArgs.size()),
                    node->getLocation());
            }

            for (size_t i = 0; i < genericTypeParams.size(); ++i)
            {
                typeBindings[genericTypeParams[i]] = genericTypeArgs[i];
            }
        }
        else
        {
            // Tier 2: Infer from function arguments
            const auto& arguments = node->getArguments();

            // Only attempt argument inference if we have both parameters and arguments
            if (!funcMetadata->parameterTypes.empty() && !arguments.empty())
            {
                inferFromArguments(funcMetadata, arguments, typeBindings);
            }

            // Tier 3: Infer from return type using expected type context
            inferFromReturnType(funcMetadata, typeBindings, node->getLocation());

            // Check if all type parameters have been inferred
            const auto& genericTypeParams = funcMetadata->genericTypeParameters;
            std::vector<std::string> uninferredParams;

            for (const auto& param : genericTypeParams)
            {
                if (typeBindings.find(param) == typeBindings.end())
                {
                    uninferredParams.push_back(param);
                }
            }

            if (!uninferredParams.empty())
            {
                // Build helpful error message
                std::string paramList;
                for (size_t i = 0; i < uninferredParams.size(); ++i)
                {
                    if (i > 0) paramList += ", ";
                    paramList += "'" + uninferredParams[i] + "'";
                }

                throw errors::TypeException(
                    "Cannot infer type parameter(s) " + paramList + " for function '" +
                    functionName + "'. " +
                    "Provide explicit type arguments like '" + functionName + "<Type>(...)'",
                    node->getLocation());
            }
        }

        // Push bindings onto stack
        ctx.pushGenericTypeBindings(typeBindings);
        return true;
    }

    void FunctionCallHelper::inferFromArguments(
        const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata,
        const std::vector<std::unique_ptr<ast::ASTNode>>& arguments,
        std::unordered_map<std::string, std::string>& typeBindings
    )
    {
        if (!funcMetadata || funcMetadata->genericTypeParameters.empty())
        {
            return;
        }

        const auto& genericTypeParams = funcMetadata->genericTypeParameters;

        // PHASE 3: Use pattern matching for nested generic inference
        if (funcMetadata->hasNestedTypeParameters())
        {
            // Build set of declared type parameters
            std::unordered_set<std::string> declaredTypeParams(
                genericTypeParams.begin(),
                genericTypeParams.end()
            );

            // Collect argument type strings
            std::vector<std::string> argumentTypes;
            for (const auto& arg : arguments)
            {
                std::string argType = inferTypeFromArgument(arg.get());
                argumentTypes.push_back(argType);
            }

            // Use pattern analyzer to extract bindings
            types::GenericPatternAnalyzer analyzer;
            std::unordered_map<std::string, std::string> patternBindings =
                analyzer.inferTypeBindings(
                    funcMetadata->parameterTypes,
                    argumentTypes,
                    declaredTypeParams
                );

            // Merge pattern bindings into typeBindings
            for (const auto& [param, type] : patternBindings)
            {
                auto it = typeBindings.find(param);
                if (it == typeBindings.end())
                {
                    typeBindings[param] = type;
                }
                else if (it->second != type)
                {
                    throw errors::TypeException(
                        "Type parameter '" + param + "' inferred as both '" +
                        it->second + "' and '" + type + "'"
                    );
                }
            }
        }
        else
        {
            // Original logic for simple (non-nested) generic inference
            for (const auto& typeParam : genericTypeParams)
            {
                std::optional<std::string> unifiedType;

                for (size_t i = 0; i < funcMetadata->parameterTypes.size(); ++i)
                {
                    const std::string& paramType = funcMetadata->parameterTypes[i];

                    if (paramType == typeParam && i < arguments.size())
                    {
                        std::string argType = inferTypeFromArgument(arguments[i].get());

                        if (!argType.empty())
                        {
                            if (!unifiedType.has_value())
                            {
                                unifiedType = argType;
                            }
                            else if (unifiedType.value() != argType)
                            {
                                throw errors::TypeException(
                                    "Type parameter '" + typeParam + "' cannot be both '" +
                                    unifiedType.value() + "' and '" + argType + "'"
                                );
                            }
                        }
                    }
                }

                if (unifiedType.has_value())
                {
                    auto it = typeBindings.find(typeParam);
                    if (it == typeBindings.end())
                    {
                        typeBindings[typeParam] = unifiedType.value();
                    }
                    else if (it->second != unifiedType.value())
                    {
                        throw errors::TypeException(
                            "Type parameter '" + typeParam + "' inferred as both '" +
                            it->second + "' and '" + unifiedType.value() + "'"
                        );
                    }
                }
            }
        }
    }

    void FunctionCallHelper::inferFromReturnType(
        const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata,
        std::unordered_map<std::string, std::string>& typeBindings,
        const ast::SourceLocation& location
    )
    {
        // PHASE 3: Infer type parameters from return type using expected type context
        if (!ctx.hasExpectedTypeContext())
        {
            return;  // No expected type available
        }

        if (!funcMetadata || funcMetadata->genericTypeParameters.empty())
        {
            return;
        }

        types::ExpectedTypeContext expectedCtx = ctx.getCurrentExpectedTypeContext();

        // Only works for object types with class names
        if (expectedCtx.expectedType != value::ValueType::OBJECT ||
            expectedCtx.expectedClassName.empty())
        {
            return;
        }

        const std::string& returnType = funcMetadata->returnType;

        // Build set of declared type parameters
        std::unordered_set<std::string> declaredTypeParams;
        for (const auto& param : funcMetadata->genericTypeParameters)
        {
            declaredTypeParams.insert(param);
        }

        // Use pattern matching to extract bindings from return type
        types::GenericPatternAnalyzer analyzer;
        std::vector<types::TypeBinding> returnBindings =
            analyzer.matchPattern(returnType, expectedCtx.expectedClassName, declaredTypeParams);

        // Merge return type bindings
        for (const auto& binding : returnBindings)
        {
            auto it = typeBindings.find(binding.parameterName);
            if (it == typeBindings.end())
            {
                // New binding from return type
                typeBindings[binding.parameterName] = binding.concreteType;
            }
            else if (it->second != binding.concreteType)
            {
                // Conflict between argument inference and return type inference
                throw errors::TypeException(
                    "Type parameter '" + binding.parameterName +
                    "' inferred as '" + it->second + "' from arguments " +
                    "but expected type suggests '" + binding.concreteType + "'",
                    location
                );
            }
            // else: same binding, no conflict
        }
    }

    void FunctionCallHelper::validateFunctionParameters(ast::FunctionCallNode* node, const std::string& functionName,
                                                        const std::vector<std::unique_ptr<ast::ASTNode>>& arguments)
    {
        // Skip validation for method calls (contain "::")
        if (functionName.find("::") != std::string::npos)
        {
            return;
        }

        // Check if function has multiple overloads - if so, skip validation here
        // Overload resolution will handle selecting the correct overload
        auto funcRegistry = ctx.environment->getFunctionRegistry();
        if (funcRegistry)
        {
            auto overloads = funcRegistry->getAllFunctionOverloads(functionName);

          

            if (overloads.size() > 1)
            {
                return;
            }
        }

        // Check if function is registered
        const auto* funcMetadata = ctx.program.getFunction(functionName);
        if (!funcMetadata)
        {
            return;
        }

        // Skip all validation for native functions (they handle their own parameter checking at runtime)
        if (funcMetadata->isNative)
        {
            return;
        }

        if (funcMetadata->parameterCount != arguments.size())
        {
            throw errors::EnvironmentException(
                "Function '" + functionName + "' expects " +
                std::to_string(funcMetadata->parameterCount) +
                " parameter(s) but got " + std::to_string(arguments.size()),
                node->getLocation()
            );
        }

        // Validate parameter types
        if (funcMetadata->parameterTypes.empty())
        {
            return;
        }

        for (size_t i = 0; i < arguments.size(); ++i)
        {
            std::string expectedType = funcMetadata->parameterTypes[i];

            // Resolve generic type parameters if present
            expectedType = ctx.resolveGenericType(expectedType);

            value::ValueType argType = ctx.typeInference.inferExpressionType(arguments[i].get());

            // Skip validation if type inference failed (returned void/unknown)
            if (argType == value::ValueType::VOID)
            {
                continue; // Can't validate - inference failed or expression is void
            }

            // Null safety enforcement: reject nullable values passed to non-nullable parameters
            bool expectedIsNullable = (i < funcMetadata->parameterNullable.size() && funcMetadata->parameterNullable[i]);
            if (!expectedIsNullable)
            {
                // Skip check for generic type parameters (T, K, V, etc.)
                bool isGenericParam = ::types::TypeConversionUtils::isGenericTypeParameter(expectedType);
                if (!isGenericParam && ctx.typeInference.inferExpressionNullable(arguments[i].get()))
                {
                    throw errors::TypeException(
                        "Cannot pass nullable value to non-nullable parameter " + std::to_string(i + 1) +
                        " of '" + functionName + "'. Parameter type is '" + expectedType +
                        "', use '" + expectedType + "?' to allow null.",
                        node->getLocation()
                    );
                }
            }

            // Convert argType to string for comparison
            std::string argTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(argType);

            // For object types, need to check class names
            if (expectedType != "int" && expectedType != "float" &&
                expectedType != "string" && expectedType != "bool" &&
                expectedType != "void")
            {
                // Special handling for array types
                bool expectedIsArray = (expectedType.find("[]") != std::string::npos ||
                                       expectedType.find("Array<") == 0);
                if (expectedIsArray && argType == value::ValueType::ARRAY)
                {
                    // Array type matches - skip detailed validation for now
                    continue;
                }

                // Expected type is an object/class
                if (argType != value::ValueType::OBJECT)
                {
                    // null can be passed to object types
                    if (!dynamic_cast<ast::NullNode*>(arguments[i].get()))
                    {
                        // PHASE 4: Allow primitive literals for Box types (auto-boxing)
                        bool canAutoBox = false;
                        if ((expectedType == "Int" && argType == value::ValueType::INT) ||
                            (expectedType == "Float" && argType == value::ValueType::FLOAT) ||
                            (expectedType == "Bool" && argType == value::ValueType::BOOL) ||
                            (expectedType == "String" && argType == value::ValueType::STRING))
                        {
                            canAutoBox = true;
                        }

                        if (!canAutoBox)
                        {
                            throw errors::TypeException(
                                "Function '" + functionName + "' parameter " + std::to_string(i + 1) +
                                " expects " + expectedType + " but got " + argTypeStr,
                                node->getLocation()
                            );
                        }
                    }
                }
                // For objects, check class name compatibility
                else if (expectedType != "object")
                {
                    std::string argClassName = ctx.typeInference.inferExpressionClassName(arguments[i].get());

                    // Normalize array types: convert "int[]" to "Array<int>", "int[][]" to "Array<Array<int>>", etc.
                    auto normalizeArrayType = [](const std::string& type) -> std::string
                    {
                        std::string normalized = type;
                        size_t arrayDepth = 0;

                        // Count array brackets from the end
                        while (normalized.length() >= 2 && normalized.substr(normalized.length() - 2) == "[]")
                        {
                            arrayDepth++;
                            normalized = normalized.substr(0, normalized.length() - 2);
                        }

                        // Wrap in Array<> for each dimension
                        for (size_t i = 0; i < arrayDepth; ++i)
                        {
                            normalized = "Array<" + normalized + ">";
                        }

                        return normalized;
                    };

                    std::string normalizedArgClassName = normalizeArrayType(argClassName);
                    std::string normalizedExpectedType = normalizeArrayType(expectedType);

                    // Strip nullable suffix for type compatibility checks
                    // Non-nullable values are always assignable to nullable parameters
                    normalizedExpectedType = ::types::TypeConversionUtils::stripNullable(normalizedExpectedType);

                    if (!argClassName.empty() && normalizedArgClassName != normalizedExpectedType)
                    {
                        // Check if both are generic types with the same base
                        bool isGenericMatch = false;
                        size_t expectedAngle = normalizedExpectedType.find('<');
                        size_t argAngle = normalizedArgClassName.find('<');

                        if (expectedAngle != std::string::npos && argAngle != std::string::npos)
                        {
                            // Both are generic types - check if base types match
                            std::string expectedBase = normalizedExpectedType.substr(0, expectedAngle);
                            std::string argBase = normalizedArgClassName.substr(0, argAngle);
                            if (expectedBase == argBase)
                            {
                                // Same generic base type (e.g., both are List, both are Array)
                                // Type argument compatibility will be validated at runtime
                                isGenericMatch = true;
                            }
                        }

                        if (!isGenericMatch)
                        {
                            // Check if argClassName is assignable to expectedType (inheritance/polymorphism)
                            // Strip nullable suffix - non-null is always compatible with nullable
                            std::string resolvedExpected = ::types::TypeConversionUtils::stripNullable(expectedType);
                            if (!ctx.typeValidator.isClassCompatible(argClassName, resolvedExpected))
                            {
                                // null can be passed to any object type
                                if (!dynamic_cast<ast::NullNode*>(arguments[i].get()))
                                {
                                    throw errors::TypeException(
                                        "Function '" + functionName + "' parameter " + std::to_string(i + 1) +
                                        " expects " + expectedType + " but got " + argClassName,
                                        node->getLocation()
                                    );
                                }
                            }
                        }
                    }
                }
            }
            // For primitive types, check exact match (with int->float exception)
            else if (expectedType != argTypeStr)
            {
                // Allow int to float conversion
                if (!(expectedType == "float" && argTypeStr == "int"))
                {
                    throw errors::TypeException(
                        "Function '" + functionName + "' parameter " + std::to_string(i + 1) +
                        " expects " + expectedType + " but got " + argTypeStr,
                        node->getLocation()
                    );
                }
            }
        }
    }

    void FunctionCallHelper::emitStaticMethodCall(ast::FunctionCallNode* node, const std::string& functionName,
                                                  const std::vector<std::unique_ptr<ast::ASTNode>>& arguments)
    {
        // Create mutable copy for potential modification
        std::string actualFunctionName = functionName;

        // Replace "this::" with actual class name if inside a class
        if (actualFunctionName.substr(0, 6) == "this::" && ctx.currentClassNode)
        {
            std::string methodName = actualFunctionName.substr(6); // Remove "this::"
            actualFunctionName = ctx.currentClassNode->getClassName() + "::" + methodName;
        }

        // Extract className and methodName from qualified name for overload resolution
        std::string className;
        std::string methodName;
        size_t colonPos = actualFunctionName.find("::");
        if (colonPos != std::string::npos)
        {
            className = actualFunctionName.substr(0, colonPos);
            methodName = actualFunctionName.substr(colonPos + 2);
        }

        // Validate static method exists at compile time
        if (ctx.compileTimeValidator && !className.empty())
        {
            ctx.compileTimeValidator->validateStaticMethodExists(className, methodName,
                                                                 arguments.size(), node->getLocation());
        }

        // Resolve method overload to get mangled name (with signature and $static suffix)
        std::string resolvedMethodName = actualFunctionName;
        if (!className.empty())
        {
            resolvedMethodName = overloadResolver->resolveStaticMethodOverload(
                className, methodName, arguments, node->getLocation());
        }

        // Compile all arguments (left to right) with auto-boxing support
        // Note: For static method calls, we don't have easy access to parameter types
        // Auto-boxing will be handled at the parser level or via explicit conversion
        // For now, compile arguments normally
        for (const auto& arg : arguments)
        {
            arg->accept(ctx.visitor);
        }

        // MYT-197: bake the $static suffix at compile time so the runtime
        // CALL_STATIC handler doesn't rebuild the string on every call.
        // resolveStaticMethodOverload already appends $static on the happy
        // path; the guard covers the className-empty fallback above where
        // resolvedMethodName was left as actualFunctionName unchanged.
        if (resolvedMethodName.find("$static") == std::string::npos)
        {
            resolvedMethodName += "$static";
        }
        size_t nameIndex = ctx.program.getConstantPool().addString(resolvedMethodName);
        // Static method call - use CALL_STATIC with source location
        ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_STATIC,
                                     static_cast<uint64_t>(nameIndex),
                                     static_cast<uint64_t>(arguments.size()), node);
    }

    void FunctionCallHelper::emitMethodCallInClassContext(ast::FunctionCallNode* node, const std::string& functionName,
                                                          const std::vector<std::unique_ptr<ast::ASTNode>>& arguments)
    {
        // Unqualified call inside a method (instance or static) - could be either:
        // 1. Method call on 'this' (for instance methods)
        // 2. Static method call (for static methods)
        // 3. Regular function call (global function)
        bool isMethodCall = false;
        bool isStaticMethodCall = false;
        const auto& methods = ctx.currentClassNode->getMethods();

        for (const auto& method : methods)
        {
            if (auto* methodNode = dynamic_cast<ast::MethodNode*>(method.get()))
            {
                if (methodNode->getName() == functionName &&
                    methodNode->getParameterCount() == arguments.size())
                {
                    // If we're in a static method, only match static methods
                    // If we're in an instance method, prefer instance methods
                    if (ctx.inStaticMethod)
                    {
                        if (methodNode->getIsStatic())
                        {
                            isMethodCall = true;
                            isStaticMethodCall = true;
                            break;
                        }
                    }
                    else if (ctx.inInstanceMethod)
                    {
                        if (!methodNode->getIsStatic())
                        {
                            isMethodCall = true;
                            isStaticMethodCall = false;
                            break;
                        }
                    }
                }
            }
        }

        // If we didn't find a matching method in the preferred namespace,
        // check the other namespace (only in instance context, static can also call instance)
        if (!isMethodCall && ctx.inInstanceMethod)
        {
            for (const auto& method : methods)
            {
                if (auto* methodNode = dynamic_cast<ast::MethodNode*>(method.get()))
                {
                    if (methodNode->getName() == functionName &&
                        methodNode->getParameterCount() == arguments.size() &&
                        methodNode->getIsStatic())
                    {
                        isMethodCall = true;
                        isStaticMethodCall = true;
                        break;
                    }
                }
            }
        }

        if (isMethodCall)
        {
            if (isStaticMethodCall)
            {
                // Static method call - use CALL_STATIC with fully qualified name
                std::string className = ctx.currentClassNode->getClassName();

                // Resolve method overload to get mangled name (includes parameter signature)
                std::string resolvedMethodName = overloadResolver->resolveStaticMethodOverload(
                    className, functionName, arguments, node->getLocation());

                // Validate static method exists at compile time
                if (ctx.compileTimeValidator)
                {
                    ctx.compileTimeValidator->validateStaticMethodExists(
                        className, functionName,
                        arguments.size(), node->getLocation());
                }

                // Compile all arguments
                for (const auto& arg : arguments)
                {
                    arg->accept(ctx.visitor);
                }

                // MYT-197: $static suffix must live in the constant pool —
                // the runtime CALL_STATIC handler no longer appends it.
                // resolveStaticMethodOverload appends on the happy path; the
                // guard covers any bypass.
                if (resolvedMethodName.find("$static") == std::string::npos)
                {
                    resolvedMethodName += "$static";
                }
                size_t nameIndex = ctx.program.getConstantPool().addString(resolvedMethodName);
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_STATIC,
                                             static_cast<uint64_t>(nameIndex),
                                             static_cast<uint64_t>(arguments.size()), node);
            }
            else
            {
                // Instance method call - push 'this' onto stack BEFORE arguments

                // Validate instance method exists at compile time
                if (ctx.compileTimeValidator)
                {
                    ctx.compileTimeValidator->validateInstanceMethodExists(
                        ctx.currentClassNode->getClassName(), functionName,
                        arguments.size(), node->getLocation());
                }

                ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(0), node);

                // Now compile arguments
                for (const auto& arg : arguments)
                {
                    arg->accept(ctx.visitor);
                }

                size_t nameIndex = ctx.program.getConstantPool().addString(functionName);
                // Call method on 'this' with source location
                // 'this' is always non-null, so set the nonnull flag
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint64_t>(nameIndex),
                                             static_cast<uint64_t>(arguments.size()), node);
                ctx.program.setLastInstructionFlags(bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER);
            }
        }
        else
        {
            // Regular function call
            emitRegularFunctionCall(node, functionName, arguments);
        }
    }

    void FunctionCallHelper::emitRegularFunctionCall(ast::FunctionCallNode* node, const std::string& resolvedFunctionName,
                                                     const std::vector<std::unique_ptr<ast::ASTNode>>& arguments)
    {
        // Extract plain function name from resolved name for validation
        // resolvedFunctionName could be "process/T" or just "process"
        std::string plainFunctionName = resolvedFunctionName;
        size_t slashPos = resolvedFunctionName.find('/');
        if (slashPos != std::string::npos)
        {
            plainFunctionName = resolvedFunctionName.substr(0, slashPos);
        }

        // Validate function exists at compile time (all functions are pre-registered)
        if (ctx.compileTimeValidator)
        {
            std::string currentClassName = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
            ctx.compileTimeValidator->validateFunctionExists(plainFunctionName, node->getLocation(), currentClassName);
        }

        // Get function metadata for parameter type information (for auto-boxing)
        // Use resolved name for metadata lookup (includes signature for overloads)
        const auto* funcMetadata = ctx.program.getFunction(resolvedFunctionName);

        // Compile all arguments (left to right) with auto-boxing/unboxing support
        for (size_t i = 0; i < arguments.size(); ++i)
        {
            // PHASE 4: Apply auto-boxing/unboxing if function metadata available
            if (funcMetadata && i < funcMetadata->parameterTypes.size())
            {
                std::string expectedType = ctx.resolveGenericType(funcMetadata->parameterTypes[i]);
                value::ValueType expectedValueType = ::types::TypeConversionUtils::stringToValueType(expectedType);

                compileArgumentWithAutoBoxing(arguments[i].get(), expectedType, expectedValueType);
            }
            else
            {
                // No metadata - but still try auto-unboxing for common native functions
                // For functions like print(), we want to auto-unbox String objects
                value::ValueType argType = ctx.typeInference.inferExpressionType(arguments[i].get());

                if (argType == value::ValueType::OBJECT)
                {
                    std::string argClassName = ctx.typeInference.inferExpressionClassName(arguments[i].get());

                    // Auto-unbox Box types for native functions (EXCEPT String)
                    // IMPORTANT: Don't auto-unbox String objects because:
                    // 1. String concatenation with mixed types may return primitive strings
                    // 2. Native functions like print() handle both primitive and Box strings
                    if (argClassName == "Int" || argClassName == "Float" || argClassName == "Bool")
                    {
                        // Compile argument (Box object)
                        arguments[i]->accept(ctx.visitor);

                        // Call getValue() to extract primitive
                        size_t methodNameIndex = ctx.program.getConstantPool().addString("getValue");
                        ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                                     static_cast<uint64_t>(methodNameIndex),
                                                     0u,  // 0 arguments
                                                     arguments[i].get());
                    }
                    else
                    {
                        // Not a Box type (or is String), compile normally
                        arguments[i]->accept(ctx.visitor);
                    }
                }
                else
                {
                    // Compile normally
                    arguments[i]->accept(ctx.visitor);
                }
            }
        }

        // Check if this is actually a static method of the current class
        bool isStaticMethodOfCurrentClass = false;
        if (ctx.currentClassNode)
        {
            std::string currentClassName = ctx.currentClassNode->getClassName();
            auto classRegistry = ctx.environment->getClassRegistry();
            auto classDef = classRegistry->findClass(currentClassName);
            if (classDef)
            {
                const auto& staticMethods = classDef->getStaticMethods();
                if (staticMethods.find(plainFunctionName) != staticMethods.end())
                {
                    isStaticMethodOfCurrentClass = true;
                }
            }
        }

        if (isStaticMethodOfCurrentClass)
        {
            // Emit CALL_STATIC for static method of current class (use plain name, not resolved).
            // MYT-197: bake the $static suffix at compile time — the runtime
            // CALL_STATIC handler no longer appends it on every call. Mirrors
            // the guard in emitStaticMethodCall. The suffix must be here (not
            // deferred to runtime) because removing the runtime concat was the
            // point of the MYT-197 change.
            std::string qualifiedName = ctx.currentClassNode->getClassName() + "::" + plainFunctionName + "$static";
            size_t nameIndex = ctx.program.getConstantPool().addString(qualifiedName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_STATIC,
                                         static_cast<uint64_t>(nameIndex),
                                         static_cast<uint64_t>(arguments.size()), node);
        }
        else
        {
            // Try CALL_FAST for statically-resolved bytecode functions
            const auto* funcMeta = ctx.program.getFunction(resolvedFunctionName);
            size_t funcIndex = ctx.program.getFunctionIndex(resolvedFunctionName);

            if (funcMeta && !funcMeta->isNative && funcIndex != SIZE_MAX)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_FAST,
                                             static_cast<uint64_t>(funcIndex),
                                             static_cast<uint64_t>(arguments.size()), node);
            }
            else
            {
                size_t nameIndex = ctx.program.getConstantPool().addString(resolvedFunctionName);
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL,
                                             static_cast<uint64_t>(nameIndex),
                                             static_cast<uint64_t>(arguments.size()), node);
            }
        }
    }

    value::Value FunctionCallHelper::compileFunctionCall(ast::FunctionCallNode* node)
    {
        // Get function name and arguments
        std::string functionName = node->getFunctionName();
        const auto& arguments = node->getArguments();

        // For regular function calls with overloads, resolve the overload FIRST
        // This is necessary for generic type binding setup to work correctly
        std::string resolvedFunctionName = functionName;
        if (functionName.find("::") == std::string::npos &&
            !(ctx.inInstanceMethod || ctx.inStaticMethod))
        {
            // This is a regular function call - resolve overload to get mangled name
            bool hasGenericTypeArgs = node->hasGenericTypeArguments();
            std::vector<std::string> genericTypeArgs = hasGenericTypeArgs ? node->getGenericTypeArguments() : std::vector<std::string>{};
            resolvedFunctionName = overloadResolver->resolveFunctionOverload(
                functionName, arguments, node->getLocation(), hasGenericTypeArgs, genericTypeArgs);
        }

        // Setup generic type bindings if needed (use resolved name for correct metadata lookup)
        bool hasGenericBindings = setupGenericTypeBindings(node, resolvedFunctionName);

        // PHASE 3: Cache resolved return type for generic functions while bindings are active
        if (hasGenericBindings)
        {
            const auto* funcMetadata = ctx.program.getFunction(resolvedFunctionName);
            if (funcMetadata && !funcMetadata->genericTypeParameters.empty())
            {
                std::string returnType = funcMetadata->returnType;

                // Resolve the return type using current bindings (while they're still on the stack)
                if (returnType != "int" && returnType != "float" &&
                    returnType != "string" && returnType != "bool" &&
                    returnType != "void" && returnType != "object")
                {
                    std::string resolvedReturnType = ctx.resolveGenericType(returnType);
                    ctx.resolvedFunctionCallTypes[node] = resolvedReturnType;
                }
            }
        }

        // Validate function parameters (count and types)
        validateFunctionParameters(node, functionName, arguments);

        // Emit appropriate bytecode based on call type
        if (functionName.find("::") != std::string::npos)
        {
            // Static method call (ClassName::methodName)
            emitStaticMethodCall(node, functionName, arguments);
        }
        else if ((ctx.inInstanceMethod || ctx.inStaticMethod) && ctx.currentClassNode)
        {
            // Unqualified call inside a method - could be instance method, static method, or function
            emitMethodCallInClassContext(node, functionName, arguments);
        }
        else
        {
            // Regular function call (use pre-resolved name to avoid double resolution)
            emitRegularFunctionCall(node, resolvedFunctionName, arguments);
        }

        // Pop generic type bindings if we pushed them
        if (hasGenericBindings)
        {
            ctx.popGenericTypeBindings();
        }

        return std::monostate{};
    }

    // Phase 4: Auto-boxing/unboxing helper for function arguments
    void FunctionCallHelper::compileArgumentWithAutoBoxing(ast::ASTNode* argument,
                                                          const std::string& expectedTypeName,
                                                          value::ValueType expectedType)
    {
        using namespace ast::nodes::expressions;

        // Get the actual type of the argument
        value::ValueType actualType = ctx.typeInference.inferExpressionType(argument);

        // PHASE 4: Auto-unboxing - if expected is primitive but argument is Box object
        if (expectedType != value::ValueType::OBJECT && actualType == value::ValueType::OBJECT)
        {
            // Check if argument is a Box type that should be unboxed
            std::string actualClassName = ctx.typeInference.inferExpressionClassName(argument);

            bool canAutoUnbox = false;
            if ((expectedType == value::ValueType::INT && actualClassName == "Int") ||
                (expectedType == value::ValueType::FLOAT && actualClassName == "Float") ||
                (expectedType == value::ValueType::BOOL && actualClassName == "Bool") ||
                (expectedType == value::ValueType::STRING && actualClassName == "String"))
            {
                canAutoUnbox = true;
            }

            if (canAutoUnbox)
            {
                // Auto-unbox: compile argument (Box object) then call getValue()
                argument->accept(ctx.visitor);

                // Call getValue() method to extract primitive value
                size_t methodNameIndex = ctx.program.getConstantPool().addString("getValue");
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint64_t>(methodNameIndex),
                                             0u,  // 0 arguments
                                             argument);
                return;
            }

            // Can't auto-unbox, compile normally
            argument->accept(ctx.visitor);
            return;
        }

        // PHASE 4: Auto-boxing - if expected is Box object but argument is primitive
        if (expectedType != value::ValueType::OBJECT)
        {
            // Not an object type, compile normally
            argument->accept(ctx.visitor);
            return;
        }

        // Check if expected type is a primitive Box type
        bool isBoxType = (expectedTypeName == "Int" ||
                          expectedTypeName == "Float" ||
                          expectedTypeName == "Bool" ||
                          expectedTypeName == "String");

        if (!isBoxType)
        {
            // Not a Box type, compile normally
            argument->accept(ctx.visitor);
            return;
        }

        // Check if argument is a primitive literal that needs boxing
        bool needsBoxing = false;

        if (expectedTypeName == "Int" && dynamic_cast<IntegerNode*>(argument))
        {
            needsBoxing = true;
        }
        else if (expectedTypeName == "Float" && dynamic_cast<FloatNode*>(argument))
        {
            needsBoxing = true;
        }
        else if (expectedTypeName == "Bool" && dynamic_cast<BoolNode*>(argument))
        {
            needsBoxing = true;
        }
        else if (expectedTypeName == "String" && dynamic_cast<StringNode*>(argument))
        {
            needsBoxing = true;
        }

        if (!needsBoxing)
        {
            // Argument is not a primitive literal, compile normally
            argument->accept(ctx.visitor);
            return;
        }

        // PHASE 4 AUTO-BOXING: Emit bytecode for boxing
        // Equivalent to: new ExpectedType(literalValue)

        // 1. Compile the literal value (pushes it onto stack)
        argument->accept(ctx.visitor);

        // 2. Emit NEW_OBJECT or NEW_VALUE_OBJECT for the Box class
        size_t classNameIndex = ctx.program.getConstantPool().addString(expectedTypeName);
        auto boxClassDef = ctx.environment->findClass(expectedTypeName);
        bool boxIsValue = boxClassDef && boxClassDef->isValueClass();
        if (boxIsValue) {
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_VALUE_OBJECT,
                                         static_cast<uint64_t>(classNameIndex),
                                         1u, argument);
            ctx.emitter.emitWithLocation(bytecode::OpCode::OBJECT_TO_VALUE, argument);
        } else {
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                         static_cast<uint64_t>(classNameIndex),
                                         1u, argument);
        }
    }
}
