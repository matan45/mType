#include "FunctionCallHelper.hpp"
#include <cstddef>
#include <cstdint>
#include "../validation/CompileTimeValidator.hpp"
#include "../../../environment/registry/SignatureUtils.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../bytecode/OpCode.hpp"

namespace vm::compiler::visitors
{
    void FunctionCallHelper::emitStaticMethodCall(ast::FunctionCallNode* node, const std::string& functionName,
                                                  const std::vector<std::unique_ptr<ast::ASTNode>>& arguments)
    {
        std::string actualFunctionName = functionName;

        // Replace "this::" with the actual class name when inside a class.
        if (actualFunctionName.substr(0, 6) == "this::" && ctx.currentClassNode)
        {
            std::string methodName = actualFunctionName.substr(6);
            actualFunctionName = ctx.currentClassNode->getClassName() + "::" + methodName;
        }

        std::string className;
        std::string methodName;
        size_t colonPos = actualFunctionName.find("::");
        if (colonPos != std::string::npos)
        {
            className = actualFunctionName.substr(0, colonPos);
            methodName = actualFunctionName.substr(colonPos + 2);
        }

        if (ctx.compileTimeValidator && !className.empty())
        {
            ctx.compileTimeValidator->validateStaticMethodExists(className, methodName,
                                                                 arguments.size(), node->getLocation());
        }

        std::string resolvedMethodName = actualFunctionName;
        if (!className.empty())
        {
            resolvedMethodName = overloadResolver->resolveStaticMethodOverload(
                className, methodName, arguments, node->getLocation());
        }

        for (const auto& arg : arguments)
        {
            arg->accept(ctx.visitor);
        }

        emitBindTypeArgsIfNeeded(node);

        // MYT-197: bake the $static suffix at compile time so the runtime
        // CALL_STATIC handler doesn't rebuild the string on every call.
        // resolveStaticMethodOverload appends $static on the happy path;
        // ensureStaticSuffix covers the className-empty fallback above.
        runtimeTypes::klass::SignatureUtils::ensureStaticSuffix(resolvedMethodName);
        size_t nameIndex = ctx.program.getConstantPool().addString(resolvedMethodName);
        ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_STATIC,
                                     static_cast<uint64_t>(nameIndex),
                                     static_cast<uint64_t>(arguments.size()), node);
    }

    void FunctionCallHelper::emitMethodCallInClassContext(ast::FunctionCallNode* node, const std::string& functionName,
                                                          const std::vector<std::unique_ptr<ast::ASTNode>>& arguments)
    {
        // Unqualified call inside a method: could be method-on-this (instance),
        // static method on current class, or global function.
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
                    // Static context: only match static. Instance context: prefer instance.
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

        // Instance context can also call static methods — check the other namespace.
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
                std::string className = ctx.currentClassNode->getClassName();

                std::string resolvedMethodName = overloadResolver->resolveStaticMethodOverload(
                    className, functionName, arguments, node->getLocation());

                if (ctx.compileTimeValidator)
                {
                    ctx.compileTimeValidator->validateStaticMethodExists(
                        className, functionName,
                        arguments.size(), node->getLocation());
                }

                for (const auto& arg : arguments)
                {
                    arg->accept(ctx.visitor);
                }

                emitBindTypeArgsIfNeeded(node);

                runtimeTypes::klass::SignatureUtils::ensureStaticSuffix(resolvedMethodName);
                size_t nameIndex = ctx.program.getConstantPool().addString(resolvedMethodName);
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_STATIC,
                                             static_cast<uint64_t>(nameIndex),
                                             static_cast<uint64_t>(arguments.size()), node);
            }
            else
            {
                if (ctx.compileTimeValidator)
                {
                    ctx.compileTimeValidator->validateInstanceMethodExists(
                        ctx.currentClassNode->getClassName(), functionName,
                        arguments.size(), node->getLocation());
                }

                // Instance method: push 'this' before arguments.
                ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint64_t>(0), node);

                for (const auto& arg : arguments)
                {
                    arg->accept(ctx.visitor);
                }

                emitBindTypeArgsIfNeeded(node);

                size_t nameIndex = ctx.program.getConstantPool().addString(functionName);
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint64_t>(nameIndex),
                                             static_cast<uint64_t>(arguments.size()), node);
                // 'this' is always non-null.
                ctx.program.setLastInstructionFlags(bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER);
            }
        }
        else
        {
            emitRegularFunctionCall(node, functionName, arguments);
        }
    }

    void FunctionCallHelper::emitRegularFunctionCall(ast::FunctionCallNode* node, const std::string& resolvedFunctionName,
                                                     const std::vector<std::unique_ptr<ast::ASTNode>>& arguments)
    {
        // Extract plain function name from resolved name (e.g. "process/T" -> "process").
        std::string plainFunctionName = resolvedFunctionName;
        size_t slashPos = resolvedFunctionName.find('/');
        if (slashPos != std::string::npos)
        {
            plainFunctionName = resolvedFunctionName.substr(0, slashPos);
        }

        if (ctx.compileTimeValidator)
        {
            std::string currentClassName = ctx.currentClassNode ? ctx.currentClassNode->getClassName() : "";
            ctx.compileTimeValidator->validateFunctionExists(plainFunctionName, node->getLocation(), currentClassName);
        }

        const auto* funcMetadata = ctx.program.getFunction(resolvedFunctionName);

        for (size_t i = 0; i < arguments.size(); ++i)
        {
            if (funcMetadata && i < funcMetadata->parameterTypes.size())
            {
                std::string expectedType = ctx.resolveGenericType(funcMetadata->parameterTypes[i]);
                value::ValueType expectedValueType = ::types::TypeConversionUtils::stringToValueType(expectedType);

                compileArgumentWithAutoBoxing(arguments[i].get(), expectedType, expectedValueType);
            }
            else
            {
                value::ValueType argType = ctx.typeInference.inferExpressionType(arguments[i].get());

                if (argType == value::ValueType::OBJECT)
                {
                    std::string argClassName = ctx.typeInference.inferExpressionClassName(arguments[i].get());

                    // Auto-unbox Box types for native functions (EXCEPT String):
                    // 1. String concatenation with mixed types may return primitive strings.
                    // 2. Native functions like print() handle both primitive and Box strings.
                    if (argClassName == "Int" || argClassName == "Float" || argClassName == "Bool")
                    {
                        arguments[i]->accept(ctx.visitor);

                        size_t methodNameIndex = ctx.program.getConstantPool().addString("getValue");
                        ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                                     static_cast<uint64_t>(methodNameIndex),
                                                     0u,
                                                     arguments[i].get());
                    }
                    else
                    {
                        arguments[i]->accept(ctx.visitor);
                    }
                }
                else
                {
                    arguments[i]->accept(ctx.visitor);
                }
            }
        }

        bool isStaticMethodOfCurrentClass = false;
        if (ctx.currentClassNode)
        {
            std::string currentClassName = ctx.currentClassNode->getClassName();
            auto classRegistry = ctx.env->getClassRegistry();
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

        // MYT-228: all three terminal CALL_* paths below trigger pushCallFrame
        // which consumes ExecutionContext::pendingTypeArgs.
        emitBindTypeArgsIfNeeded(node);

        if (isStaticMethodOfCurrentClass)
        {
            // MYT-197: bake $static at compile time (use plain name, not resolved).
            std::string qualifiedName = ctx.currentClassNode->getClassName() + "::" + plainFunctionName + "$static";
            size_t nameIndex = ctx.program.getConstantPool().addString(qualifiedName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_STATIC,
                                         static_cast<uint64_t>(nameIndex),
                                         static_cast<uint64_t>(arguments.size()), node);
        }
        else
        {
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

    void FunctionCallHelper::compileArgumentWithAutoBoxing(ast::ASTNode* argument,
                                                          const std::string& expectedTypeName,
                                                          value::ValueType expectedType)
    {
        using namespace ast::nodes::expressions;

        value::ValueType actualType = ctx.typeInference.inferExpressionType(argument);

        // PHASE 4 auto-unboxing: expected is primitive but argument is a Box object.
        if (expectedType != value::ValueType::OBJECT && actualType == value::ValueType::OBJECT)
        {
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
                argument->accept(ctx.visitor);

                size_t methodNameIndex = ctx.program.getConstantPool().addString("getValue");
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint64_t>(methodNameIndex),
                                             0u,
                                             argument);
                return;
            }

            argument->accept(ctx.visitor);
            return;
        }

        if (expectedType != value::ValueType::OBJECT)
        {
            argument->accept(ctx.visitor);
            return;
        }

        bool isBoxType = (expectedTypeName == "Int" ||
                          expectedTypeName == "Float" ||
                          expectedTypeName == "Bool" ||
                          expectedTypeName == "String");

        if (!isBoxType)
        {
            // Lambdas need the target type to infer SAM parameter types when
            // passed directly as function arguments.
            bool pushedExpectedType = false;
            if (!::types::TypeConversionUtils::containsGenericTypeParameter(expectedTypeName))
            {
                types::ExpectedTypeContext expectedCtx(expectedType, expectedTypeName);
                ctx.pushExpectedTypeContext(expectedCtx);
                pushedExpectedType = true;
            }
            argument->accept(ctx.visitor);
            if (pushedExpectedType)
            {
                ctx.popExpectedTypeContext();
            }
            return;
        }

        bool needsBoxing = false;

        if (expectedTypeName == "Int" && dynamic_cast<IntegerNode*>(argument)) needsBoxing = true;
        else if (expectedTypeName == "Float" && dynamic_cast<FloatNode*>(argument)) needsBoxing = true;
        else if (expectedTypeName == "Bool" && dynamic_cast<BoolNode*>(argument)) needsBoxing = true;
        else if (expectedTypeName == "String" && dynamic_cast<StringNode*>(argument)) needsBoxing = true;

        if (!needsBoxing)
        {
            // Not a primitive literal; compile with the expected object type so
            // lambda arguments still receive target typing.
            bool pushedExpectedType = false;
            if (!::types::TypeConversionUtils::containsGenericTypeParameter(expectedTypeName))
            {
                types::ExpectedTypeContext expectedCtx(expectedType, expectedTypeName);
                ctx.pushExpectedTypeContext(expectedCtx);
                pushedExpectedType = true;
            }
            argument->accept(ctx.visitor);
            if (pushedExpectedType)
            {
                ctx.popExpectedTypeContext();
            }
            return;
        }

        // Auto-boxing: equivalent to `new ExpectedType(literalValue)`.
        argument->accept(ctx.visitor);

        size_t classNameIndex = ctx.program.getConstantPool().addString(expectedTypeName);
        auto boxClassDef = ctx.env->findClass(expectedTypeName);
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
