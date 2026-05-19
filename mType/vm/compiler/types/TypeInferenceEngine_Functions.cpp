#include "TypeInferenceEngine.hpp"
#include <cstddef>
#include <unordered_map>
#include "../../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../../ast/nodes/functions/FunctionCallNode.hpp"

namespace vm::compiler::types
{
    const bytecode::BytecodeProgram::FunctionMetadata* TypeInferenceEngine::findOverloadMetadata(
        const std::string& callName,
        size_t argCount,
        const std::vector<std::unique_ptr<ast::ASTNode>>& arguments) const
    {
        if (const auto* exact = program.getFunction(callName)) {
            return exact;
        }

        const auto& allFunctions = program.getFunctions();
        std::string prefix = callName + "/";

        std::vector<const bytecode::BytecodeProgram::FunctionMetadata*> matchingOverloads;
        for (const auto& [name, metadata] : allFunctions) {
            if (name.find(prefix) == 0 || name == callName) {
                matchingOverloads.push_back(&metadata);
            }
        }

        if (matchingOverloads.empty()) {
            return nullptr;
        }
        if (matchingOverloads.size() == 1) {
            return matchingOverloads[0];
        }

        const bytecode::BytecodeProgram::FunctionMetadata* bestMatch = nullptr;
        int bestScore = -1;

        for (const auto* overload : matchingOverloads) {
            size_t expectedParamCount = overload->parameterCount;
            bool isInstanceMethod = !overload->isStatic;
            if (isInstanceMethod && expectedParamCount > 0) {
                expectedParamCount--;
            }

            if (expectedParamCount != argCount) {
                continue;
            }

            int score = 0;
            bool compatible = true;
            size_t paramStartIdx = isInstanceMethod ? 1 : 0;

            for (size_t i = 0; i < argCount; ++i) {
                size_t paramIdx = paramStartIdx + i;
                if (paramIdx >= overload->parameterTypes.size()) {
                    compatible = false;
                    break;
                }

                value::ValueType argType = inferExpressionType(arguments[i].get());
                const std::string& paramType = overload->parameterTypes[paramIdx];

                if ((argType == value::ValueType::INT && paramType == "int") ||
                    (argType == value::ValueType::FLOAT && paramType == "float") ||
                    (argType == value::ValueType::STRING && paramType == "string") ||
                    (argType == value::ValueType::BOOL && paramType == "bool")) {
                    score += 100;
                }
                else if (argType == value::ValueType::INT && paramType == "float") {
                    score += 50;
                }
                else if (argType == value::ValueType::OBJECT) {
                    std::string argClassName = inferExpressionClassName(arguments[i].get());
                    if (!argClassName.empty() && argClassName == paramType) {
                        score += 100;
                    } else {
                        score += 10;
                    }
                }
                else {
                    compatible = false;
                    break;
                }
            }

            if (compatible && score > bestScore) {
                bestScore = score;
                bestMatch = overload;
            }
        }

        return bestMatch ? bestMatch : matchingOverloads[0];
    }

    bool TypeInferenceEngine::isUnboundGenericReturn(
        const bytecode::BytecodeProgram::FunctionMetadata* metadata) const
    {
        if (!metadata || metadata->genericTypeParameters.empty() || metadata->returnType.empty()) {
            return false;
        }

        std::string base = metadata->returnType;
        if (!base.empty() && base.back() == '?') {
            base.pop_back();
        }
        if (base.size() >= 2 && base.compare(base.size() - 2, 2, "[]") == 0) {
            base = base.substr(0, base.size() - 2);
        }

        for (const auto& g : metadata->genericTypeParameters) {
            if (g == base) {
                return resolveGenericType(base) == base;
            }
        }
        return false;
    }

    value::ValueType TypeInferenceEngine::inferFunctionCallType(ast::FunctionCallNode* funcCall) const
    {
        const auto* funcMetadata = findOverloadMetadata(
            funcCall->getFunctionName(),
            funcCall->getArgumentCount(),
            funcCall->getArguments());
        if (isUnboundGenericReturn(funcMetadata)) {
            return value::ValueType::VOID;
        }
        if (funcMetadata && !funcMetadata->returnType.empty()) {
            const std::string& returnType = funcMetadata->returnType;
            if (returnType == "int") return value::ValueType::INT;
            if (returnType == "float") return value::ValueType::FLOAT;
            if (returnType == "string") return value::ValueType::STRING;
            if (returnType == "bool") return value::ValueType::BOOL;
            if (returnType == "void") return value::ValueType::VOID;
            if (returnType.find("[]") != std::string::npos ||
                returnType.find("Array<") == 0) {
                return value::ValueType::ARRAY;
            }
            return value::ValueType::OBJECT;
        }
        return value::ValueType::VOID;
    }

    value::ValueType TypeInferenceEngine::inferMethodCallType(ast::MethodCallNode* methodCall) const
    {
        std::string className = inferExpressionClassName(methodCall->getObject());
        if (!className.empty()) {
            std::string methodName = className + "::" + methodCall->getMethodName();
            const auto* funcMetadata = findOverloadMetadata(
                methodName,
                methodCall->getArgumentCount(),
                methodCall->getArguments());

            if (funcMetadata && !funcMetadata->returnType.empty()) {
                std::string returnType = funcMetadata->returnType;

                // Generic methods: substitute declared type parameters with the
                // call-site generic type arguments before classifying.
                if (methodCall->hasGenericTypeArguments() && !funcMetadata->genericTypeParameters.empty())
                {
                    const auto& genericTypeArgs = methodCall->getGenericTypeArguments();
                    const auto& genericTypeParams = funcMetadata->genericTypeParameters;

                    std::unordered_map<std::string, std::string> substitutions;
                    for (size_t i = 0; i < genericTypeParams.size() && i < genericTypeArgs.size(); ++i)
                    {
                        substitutions[genericTypeParams[i]] = genericTypeArgs[i];
                    }

                    if (!substitutions.empty())
                    {
                        size_t arrayPos = returnType.find('[');
                        if (arrayPos != std::string::npos)
                        {
                            std::string elementType = returnType.substr(0, arrayPos);
                            std::string arrayDimensions = returnType.substr(arrayPos);
                            auto it = substitutions.find(elementType);
                            if (it != substitutions.end())
                            {
                                returnType = it->second + arrayDimensions;
                            }
                        }
                        else
                        {
                            auto it = substitutions.find(returnType);
                            if (it != substitutions.end())
                            {
                                returnType = it->second;
                            }
                        }
                    }
                }

                if (returnType == "int") return value::ValueType::INT;
                if (returnType == "float") return value::ValueType::FLOAT;
                if (returnType == "string") return value::ValueType::STRING;
                if (returnType == "bool") return value::ValueType::BOOL;
                if (returnType == "void") return value::ValueType::VOID;

                if (returnType.find("[]") != std::string::npos ||
                    returnType.find("Array<") == 0) {
                    return value::ValueType::ARRAY;
                }
                return value::ValueType::OBJECT;
            }
        }
        return value::ValueType::VOID;
    }

    const bytecode::BytecodeProgram::FunctionMetadata* TypeInferenceEngine::findInstanceMethodMetadata(
        const std::string& className,
        ast::MethodCallNode* methodCall) const
    {
        std::string methodName = className + "::" + methodCall->getMethodName();
        if (const auto* exact = program.getFunction(methodName)) {
            return exact;
        }

        // Overloaded methods register as "ClassName::method/paramTypes"; prefix
        // match and score by argument arity + type compatibility.
        const auto& allFunctions = program.getFunctions();
        std::string prefix = methodName + "/";

        std::vector<const bytecode::BytecodeProgram::FunctionMetadata*> matchingOverloads;
        for (const auto& [name, metadata] : allFunctions) {
            if (name.find(prefix) == 0 || name == methodName) {
                matchingOverloads.push_back(&metadata);
            }
        }

        if (matchingOverloads.empty()) {
            return nullptr;
        }
        if (matchingOverloads.size() == 1) {
            return matchingOverloads[0];
        }

        size_t argCount = methodCall->getArgumentCount();
        const auto& arguments = methodCall->getArguments();

        const bytecode::BytecodeProgram::FunctionMetadata* bestMatch = nullptr;
        int bestScore = -1;

        for (const auto* overload : matchingOverloads) {
            size_t expectedParamCount = overload->parameterCount;
            bool isInstanceMethod = !overload->isStatic;
            if (isInstanceMethod && expectedParamCount > 0) {
                expectedParamCount--;
            }

            if (expectedParamCount != argCount) {
                continue;
            }

            int score = 0;
            bool compatible = true;
            size_t paramStartIdx = isInstanceMethod ? 1 : 0;

            for (size_t i = 0; i < argCount; ++i) {
                size_t paramIdx = paramStartIdx + i;
                if (paramIdx >= overload->parameterTypes.size()) {
                    compatible = false;
                    break;
                }

                value::ValueType argType = inferExpressionType(arguments[i].get());
                const std::string& paramType = overload->parameterTypes[paramIdx];

                if ((argType == value::ValueType::INT && paramType == "int") ||
                    (argType == value::ValueType::FLOAT && paramType == "float") ||
                    (argType == value::ValueType::STRING && paramType == "string") ||
                    (argType == value::ValueType::BOOL && paramType == "bool")) {
                    score += 100;
                }
                else if (argType == value::ValueType::INT && paramType == "float") {
                    score += 50;
                }
                else if (argType == value::ValueType::OBJECT) {
                    std::string argClassName = inferExpressionClassName(arguments[i].get());
                    if (!argClassName.empty() && argClassName == paramType) {
                        score += 100;
                    } else {
                        score += 10;
                    }
                }
                else {
                    compatible = false;
                    break;
                }
            }

            if (compatible && score > bestScore) {
                bestScore = score;
                bestMatch = overload;
            }
        }

        return bestMatch ? bestMatch : matchingOverloads[0];
    }
}
