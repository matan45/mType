#include "TypeInferenceEngine.hpp"
#include <cstddef>
#include <unordered_map>
#include <utility>
#include "../../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../environment/registry/MethodDefinition.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../overload/OverloadResolver.hpp"

namespace vm::compiler::types
{
    namespace rk = runtimeTypes::klass;

    namespace
    {
        std::string stripGenericArguments(std::string typeName)
        {
            typeName = ::types::TypeConversionUtils::stripNullable(typeName);
            size_t anglePos = typeName.find('<');
            if (anglePos != std::string::npos)
            {
                typeName = typeName.substr(0, anglePos);
            }
            return typeName;
        }
    }

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
        std::string className = resolveMethodCallReceiverClassName(methodCall);
        if (!className.empty()) {
            std::string methodName = className + "::" + methodCall->getMethodName();
            const auto* funcMetadata = findOverloadMetadata(
                methodName,
                methodCall->getArgumentCount(),
                methodCall->getArguments());

            if (funcMetadata && !funcMetadata->returnType.empty()) {
                return classifyReturnTypeName(applyGenericMethodReturnSubstitutions(
                    funcMetadata->returnType,
                    methodCall,
                    funcMetadata->genericTypeParameters));
            }
        }

        auto methodDef = resolveEnvironmentMethodCall(methodCall);
        if (methodDef)
        {
            return classifyReturnTypeName(getMethodDefinitionReturnTypeName(methodDef, methodCall));
        }

        return value::ValueType::VOID;
    }

    std::string TypeInferenceEngine::resolveMethodCallReceiverClassName(ast::MethodCallNode* methodCall) const
    {
        if (!methodCall)
        {
            return "";
        }

        auto* objectNode = methodCall->getObject();
        if (methodCall->getIsStaticCall())
        {
            if (auto* varNode = dynamic_cast<ast::VariableNode*>(objectNode))
            {
                std::string className = varNode->getName();
                if (className == "this" && currentClassNode)
                {
                    return currentClassNode->getClassName();
                }
                return className;
            }
        }

        if (auto* varNode = dynamic_cast<ast::VariableNode*>(objectNode))
        {
            if (varNode->getName() == "this" && currentClassNode && inInstanceMethod)
            {
                return currentClassNode->getClassName();
            }
        }

        return inferExpressionClassName(objectNode);
    }

    std::vector<value::ParameterType> TypeInferenceEngine::inferArgumentParameterTypes(
        const std::vector<std::unique_ptr<ast::ASTNode>>& arguments) const
    {
        std::vector<value::ParameterType> argTypes;
        argTypes.reserve(arguments.size());

        for (const auto& arg : arguments)
        {
            if (dynamic_cast<ast::NullNode*>(arg.get()))
            {
                argTypes.emplace_back(value::ValueType::NULL_TYPE);
                continue;
            }

            value::ValueType basicType = inferExpressionType(arg.get());
            if (basicType == value::ValueType::OBJECT)
            {
                std::string className = inferExpressionClassName(arg.get());
                if (!className.empty())
                {
                    if (environment && environment->findInterface(className))
                    {
                        argTypes.emplace_back(value::ParameterType::forInterface(className));
                    }
                    else
                    {
                        argTypes.emplace_back(value::ParameterType::forClass(className));
                    }
                }
                else
                {
                    argTypes.emplace_back(basicType);
                }
            }
            else if (basicType == value::ValueType::ARRAY)
            {
                std::string arrayTypeName = inferExpressionClassName(arg.get());
                if (!arrayTypeName.empty())
                {
                    argTypes.emplace_back(value::ParameterType::forArray(arrayTypeName));
                }
                else
                {
                    argTypes.emplace_back(basicType);
                }
            }
            else
            {
                argTypes.emplace_back(basicType);
            }
        }

        return argTypes;
    }

    std::shared_ptr<rk::MethodDefinition> TypeInferenceEngine::resolveEnvironmentMethodCall(
        ast::MethodCallNode* methodCall) const
    {
        if (!methodCall || !environment)
        {
            return nullptr;
        }

        std::string className = stripGenericArguments(resolveMethodCallReceiverClassName(methodCall));
        if (className.empty())
        {
            return nullptr;
        }

        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            return nullptr;
        }

        std::vector<std::shared_ptr<rk::MethodDefinition>> overloads =
            methodCall->getIsStaticCall()
                ? classDef->getAllStaticMethodOverloadsInHierarchy(methodCall->getMethodName())
                : classDef->getAllInstanceMethodOverloadsInHierarchy(methodCall->getMethodName());

        if (overloads.empty())
        {
            return nullptr;
        }

        if (methodCall->hasGenericTypeArguments())
        {
            std::vector<std::shared_ptr<rk::MethodDefinition>> filtered;
            for (const auto& overload : overloads)
            {
                if (overload &&
                    overload->getGenericTypeParameters().size() == methodCall->getGenericTypeArguments().size())
                {
                    filtered.push_back(overload);
                }
            }
            overloads = std::move(filtered);
        }

        if (overloads.empty())
        {
            return nullptr;
        }

        auto catalog = environment->getTypeCatalog();
        if (!catalog)
        {
            return nullptr;
        }

        std::vector<value::ParameterType> argTypes = inferArgumentParameterTypes(methodCall->getArguments());
        auto result = overload::OverloadResolver::resolveMethodOverload(
            overloads,
            argTypes,
            methodCall->getLocation(),
            *catalog);

        if (!result.isAmbiguous && result.selectedOverload)
        {
            return result.selectedOverload;
        }

        return nullptr;
    }

    std::string TypeInferenceEngine::applyGenericMethodReturnSubstitutions(
        std::string returnType,
        ast::MethodCallNode* methodCall,
        const std::vector<std::string>& genericTypeParameters) const
    {
        if (!methodCall || !methodCall->hasGenericTypeArguments() || genericTypeParameters.empty())
        {
            return returnType;
        }

        const auto& genericTypeArgs = methodCall->getGenericTypeArguments();
        std::unordered_map<std::string, std::string> substitutions;
        for (size_t i = 0; i < genericTypeParameters.size() && i < genericTypeArgs.size(); ++i)
        {
            substitutions[genericTypeParameters[i]] = genericTypeArgs[i];
        }

        if (substitutions.empty())
        {
            return returnType;
        }

        size_t arrayPos = returnType.find('[');
        if (arrayPos != std::string::npos)
        {
            std::string elementType = returnType.substr(0, arrayPos);
            std::string arrayDimensions = returnType.substr(arrayPos);
            auto it = substitutions.find(elementType);
            if (it != substitutions.end())
            {
                return it->second + arrayDimensions;
            }
            return returnType;
        }

        auto it = substitutions.find(returnType);
        if (it != substitutions.end())
        {
            return it->second;
        }

        return returnType;
    }

    std::string TypeInferenceEngine::getMethodDefinitionReturnTypeName(
        const std::shared_ptr<rk::MethodDefinition>& method,
        ast::MethodCallNode* methodCall) const
    {
        if (!method)
        {
            return "";
        }

        std::string returnType;
        if (auto unifiedReturn = method->getUnifiedReturnType())
        {
            returnType = unifiedReturn->toString();
        }
        else
        {
            returnType = ::types::TypeConversionUtils::getTypeDisplayName(method->getReturnType());
        }

        std::vector<std::string> genericParams;
        genericParams.reserve(method->getGenericTypeParameters().size());
        for (const auto& param : method->getGenericTypeParameters())
        {
            genericParams.push_back(param.name);
        }

        return applyGenericMethodReturnSubstitutions(returnType, methodCall, genericParams);
    }

    value::ValueType TypeInferenceEngine::classifyReturnTypeName(const std::string& returnType) const
    {
        std::string normalized = ::types::TypeConversionUtils::stripNullable(resolveGenericType(returnType));

        if (normalized == "int") return value::ValueType::INT;
        if (normalized == "float") return value::ValueType::FLOAT;
        if (normalized == "string") return value::ValueType::STRING;
        if (normalized == "bool") return value::ValueType::BOOL;
        if (normalized == "void") return value::ValueType::VOID;
        if (normalized.find("[]") != std::string::npos ||
            normalized.find("Array<") == 0) {
            return value::ValueType::ARRAY;
        }
        if (!normalized.empty()) return value::ValueType::OBJECT;
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
