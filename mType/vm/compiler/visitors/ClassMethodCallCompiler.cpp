#include "ClassCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include <set>
#include "GenericScopeHelper.hpp"
#include "../validation/CompileTimeValidator.hpp"
#include "../../../environment/registry/SignatureUtils.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"

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

                // Generic type arguments must be object types, not primitives
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

                // MYT-284: Validate against the full overload set, not a single
                // by-name registry lookup. The resolver filters by declared-type-arg
                // arity; here we only need to confirm some overload accepts the count.
                auto classDef = ctx.env->findClass(className);
                if (classDef) {
                    auto overloads = classDef->getAllStaticMethodOverloadsInHierarchy(methodName);
                    if (!overloads.empty()) {
                        bool anyGeneric = false;
                        bool arityMatches = false;
                        std::vector<size_t> declaredCounts;
                        declaredCounts.reserve(overloads.size());
                        for (const auto& overload : overloads) {
                            const auto& genericParams = overload->getGenericTypeParameters();
                            if (!genericParams.empty()) {
                                anyGeneric = true;
                            }
                            declaredCounts.push_back(genericParams.size());
                            if (genericParams.size() == typeArgs.size()) {
                                arityMatches = true;
                            }
                        }

                        if (!anyGeneric) {
                            throw errors::TypeException(
                                "Method '" + qualifiedName + "' is not generic but used with type arguments",
                                node->getLocation()
                            );
                        }

                        if (!arityMatches) {
                            std::set<size_t> uniqueCounts(declaredCounts.begin(), declaredCounts.end());
                            std::string expectedList;
                            bool first = true;
                            for (size_t c : uniqueCounts) {
                                if (!first) expectedList += " or ";
                                expectedList += std::to_string(c);
                                first = false;
                            }
                            throw errors::TypeException(
                                "Method '" + qualifiedName + "' expects " +
                                expectedList +
                                " type argument(s) but " + std::to_string(typeArgs.size()) + " provided",
                                node->getLocation()
                            );
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

                if (!autoBoxed)
                {
                    bool pushedExpectedType = false;
                    if (methodMetadata && !methodMetadata->isNative)
                    {
                        size_t paramOffset = (!methodMetadata->isStatic && !methodMetadata->parameterTypes.empty()) ? 1 : 0;
                        if (i + paramOffset < methodMetadata->parameterTypes.size())
                        {
                            std::string expectedType = ctx.resolveGenericType(methodMetadata->parameterTypes[i + paramOffset]);
                            value::ValueType expectedValueType = ::types::TypeConversionUtils::stringToValueType(expectedType);
                            if (expectedValueType == value::ValueType::OBJECT && !expectedType.empty() &&
                                !::types::TypeConversionUtils::containsGenericTypeParameter(expectedType))
                            {
                                ctx.pushExpectedTypeContext(types::ExpectedTypeContext(expectedValueType, expectedType));
                                pushedExpectedType = true;
                            }
                        }
                    }

                    arguments[i]->accept(ctx.visitor);

                    if (pushedExpectedType)
                    {
                        ctx.popExpectedTypeContext();
                    }
                }
            }

            // MYT-228: stage method-level type-arg bindings into the next frame
            // so `obj isClassOf T` and `(T)cast` in the callee resolve correctly.
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

            // MYT-197: $static suffix must live in the constant pool — the
            // runtime CALL_STATIC handler no longer appends it.
            runtimeTypes::klass::SignatureUtils::ensureStaticSuffix(resolvedMethodName);
            size_t methodNameIndex = ctx.program.getConstantPool().addString(resolvedMethodName);
            ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_STATIC,
                             static_cast<uint64_t>(methodNameIndex),
                             static_cast<uint64_t>(arguments.size()), node);
        }
    }
}
