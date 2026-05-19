#include "FunctionCallHelper.hpp"
#include <cstddef>
#include <cstdint>
#include "../../../errors/TypeException.hpp"
#include "../../../errors/EnvironmentException.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../types/GenericPatternAnalyzer.hpp"
#include <optional>

namespace vm::compiler::visitors
{
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

        // PHASE 3: pattern matching for nested generic inference.
        if (funcMetadata->hasNestedTypeParameters())
        {
            std::unordered_set<std::string> declaredTypeParams(
                genericTypeParams.begin(),
                genericTypeParams.end()
            );

            std::vector<std::string> argumentTypes;
            for (const auto& arg : arguments)
            {
                std::string argType = inferTypeFromArgument(arg.get());
                argumentTypes.push_back(argType);
            }

            types::GenericPatternAnalyzer analyzer;
            std::unordered_map<std::string, std::string> patternBindings =
                analyzer.inferTypeBindings(
                    funcMetadata->parameterTypes,
                    argumentTypes,
                    declaredTypeParams
                );

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
            // Simple (non-nested) generic inference.
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
        if (!ctx.hasExpectedTypeContext())
        {
            return;
        }

        if (!funcMetadata || funcMetadata->genericTypeParameters.empty())
        {
            return;
        }

        types::ExpectedTypeContext expectedCtx = ctx.getCurrentExpectedTypeContext();

        if (expectedCtx.expectedType != value::ValueType::OBJECT ||
            expectedCtx.expectedClassName.empty())
        {
            return;
        }

        const std::string& returnType = funcMetadata->returnType;

        std::unordered_set<std::string> declaredTypeParams;
        for (const auto& param : funcMetadata->genericTypeParameters)
        {
            declaredTypeParams.insert(param);
        }

        types::GenericPatternAnalyzer analyzer;
        std::vector<types::TypeBinding> returnBindings =
            analyzer.matchPattern(returnType, expectedCtx.expectedClassName, declaredTypeParams);

        for (const auto& binding : returnBindings)
        {
            auto it = typeBindings.find(binding.parameterName);
            if (it == typeBindings.end())
            {
                typeBindings[binding.parameterName] = binding.concreteType;
            }
            else if (it->second != binding.concreteType)
            {
                // MYT-282: arrays widen to Object (MYT-281). When the argument-side
                // binding is an array form ("int[]") and the return-context
                // binding is "Object", keep the more specific argument binding
                // instead of throwing a conflict — the array satisfies the Object
                // expectation by subtyping. Suffix-check (not substring): array
                // forms always end in "[]".
                auto endsWithArrayBrackets = [](const std::string& s) {
                    return s.size() >= 2 &&
                           s.compare(s.size() - 2, 2, "[]") == 0;
                };
                const bool argIsArray = endsWithArrayBrackets(it->second);
                const bool ctxIsArray = endsWithArrayBrackets(binding.concreteType);
                if (binding.concreteType == "Object" && argIsArray)
                {
                    continue;
                }
                if (it->second == "Object" && ctxIsArray)
                {
                    typeBindings[binding.parameterName] = binding.concreteType;
                    continue;
                }

                throw errors::TypeException(
                    "Type parameter '" + binding.parameterName +
                    "' inferred as '" + it->second + "' from arguments " +
                    "but expected type suggests '" + binding.concreteType + "'",
                    location
                );
            }
        }
    }

    void FunctionCallHelper::validateFunctionParameters(ast::FunctionCallNode* node, const std::string& functionName,
                                                        const std::vector<std::unique_ptr<ast::ASTNode>>& arguments)
    {
        // Skip method calls (contain "::") — those validate via the method path.
        if (functionName.find("::") != std::string::npos)
        {
            return;
        }

        // Multiple overloads: overload resolution handles selection; skip count/type
        // checks here so we don't reject a call against the first overload's signature.
        auto funcRegistry = ctx.env->getFunctionRegistry();
        if (funcRegistry)
        {
            auto overloads = funcRegistry->getAllFunctionOverloads(functionName);

            if (overloads.size() > 1)
            {
                return;
            }
        }

        const auto* funcMetadata = ctx.program.getFunction(functionName);
        if (!funcMetadata)
        {
            return;
        }

        // Native functions handle their own parameter checking at runtime.
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

        if (funcMetadata->parameterTypes.empty())
        {
            return;
        }

        for (size_t i = 0; i < arguments.size(); ++i)
        {
            std::string expectedType = funcMetadata->parameterTypes[i];

            expectedType = ctx.resolveGenericType(expectedType);

            value::ValueType argType = ctx.typeInference.inferExpressionType(arguments[i].get());

            // Skip if type inference failed (expression is void or unknown).
            if (argType == value::ValueType::VOID)
            {
                continue;
            }

            // Null-safety: reject nullable values passed to non-nullable parameters.
            bool expectedIsNullable = (i < funcMetadata->parameterNullable.size() && funcMetadata->parameterNullable[i]);
            if (!expectedIsNullable)
            {
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

            std::string argTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(argType);

            if (expectedType != "int" && expectedType != "float" &&
                expectedType != "string" && expectedType != "bool" &&
                expectedType != "void")
            {
                bool expectedIsArray = (expectedType.find("[]") != std::string::npos ||
                                       expectedType.find("Array<") == 0);
                if (expectedIsArray && argType == value::ValueType::ARRAY)
                {
                    continue;
                }

                // MYT-281: arrays widen to `Object` as the universal heap-typed slot.
                if (expectedType == "Object" && argType == value::ValueType::ARRAY)
                {
                    continue;
                }

                if (argType != value::ValueType::OBJECT)
                {
                    if (!dynamic_cast<ast::NullNode*>(arguments[i].get()))
                    {
                        // PHASE 4: allow primitive literals for Box types (auto-boxing).
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
                else if (expectedType != "object")
                {
                    std::string argClassName = ctx.typeInference.inferExpressionClassName(arguments[i].get());

                    // Normalize array types: "int[]" -> "Array<int>", "int[][]" -> "Array<Array<int>>".
                    auto normalizeArrayType = [](const std::string& type) -> std::string
                    {
                        std::string normalized = type;
                        size_t arrayDepth = 0;

                        while (normalized.length() >= 2 && normalized.substr(normalized.length() - 2) == "[]")
                        {
                            arrayDepth++;
                            normalized = normalized.substr(0, normalized.length() - 2);
                        }

                        for (size_t i = 0; i < arrayDepth; ++i)
                        {
                            normalized = "Array<" + normalized + ">";
                        }

                        return normalized;
                    };

                    std::string normalizedArgClassName = normalizeArrayType(argClassName);
                    std::string normalizedExpectedType = normalizeArrayType(expectedType);

                    // Non-nullable values are always assignable to nullable parameters.
                    normalizedExpectedType = ::types::TypeConversionUtils::stripNullable(normalizedExpectedType);

                    if (!argClassName.empty() && normalizedArgClassName != normalizedExpectedType)
                    {
                        bool isGenericMatch = false;
                        size_t expectedAngle = normalizedExpectedType.find('<');
                        size_t argAngle = normalizedArgClassName.find('<');

                        if (expectedAngle != std::string::npos && argAngle != std::string::npos)
                        {
                            // Both are generic types with the same base — argument
                            // compatibility validates at runtime.
                            std::string expectedBase = normalizedExpectedType.substr(0, expectedAngle);
                            std::string argBase = normalizedArgClassName.substr(0, argAngle);
                            if (expectedBase == argBase)
                            {
                                isGenericMatch = true;
                            }
                        }

                        if (!isGenericMatch)
                        {
                            std::string resolvedExpected = ::types::TypeConversionUtils::stripNullable(expectedType);
                            if (!ctx.typeValidator.isClassCompatible(argClassName, resolvedExpected))
                            {
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
            // Primitive types: exact match except int->float widening.
            else if (expectedType != argTypeStr)
            {
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
}
