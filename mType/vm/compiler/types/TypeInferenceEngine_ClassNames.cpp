#include "TypeInferenceEngine.hpp"
#include <cstddef>
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/expressions/CastExpression.hpp"
#include "../../../ast/nodes/expressions/AwaitExpression.hpp"
#include "../../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../../../ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../../../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../../ast/nodes/classes/NewNode.hpp"
#include "../../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../token/TokenType.hpp"

namespace vm::compiler::types
{
    std::string TypeInferenceEngine::inferCastClassName(ast::CastExpression* castExpr) const
    {
        const auto* targetType = castExpr->getTargetType();
        if (targetType) {
            std::string targetTypeName = targetType->toString();
            if (targetTypeName != "int" && targetTypeName != "float" &&
                targetTypeName != "string" && targetTypeName != "bool" &&
                targetTypeName != "void") {
                return targetTypeName;
            }
        }
        return "";
    }

    std::string TypeInferenceEngine::inferVariableClassName(ast::VariableNode* varNode) const
    {
        std::string varName = varNode->getName();

        if (varName == "this" && currentClassNode && inInstanceMethod) {
            return currentClassNode->getClassName();
        }

        // MYT-377: resolve `Class::FIELD` to the static field's class name so
        // object and object-array static fields type-check in typed positions
        // (ParameterValidator compares class names for OBJECT arguments).
        if (varName.find("::") != std::string::npos) {
            if (auto field = resolveQualifiedStaticField(varName)) {
                if (auto uType = field->getUnifiedType()) {
                    return ::types::TypeConversionUtils::stripNullable(uType->toString());
                }
            }
            return "";
        }

        const auto& locals = variableTracker.getLocals();
        for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
            if (it->name == varName) {
                return it->className;
            }
        }

        if (currentClassNode && inInstanceMethod) {
            for (const auto& field : currentClassNode->getFields()) {
                if (auto* fieldNode = dynamic_cast<ast::FieldNode*>(field.get())) {
                    if (fieldNode->getName() == varName && !fieldNode->getIsStatic()) {
                        auto genericType = fieldNode->getGenericType();
                        if (genericType) {
                            return ::types::TypeConversionUtils::stripNullable(genericType->toString());
                        }
                    }
                }
            }

            if (currentClassNode->hasParentClass()) {
                std::string parentClassName = currentClassNode->getParentClassName();
                auto parentDef = environment->getClassRegistry()->findClass(parentClassName);
                while (parentDef) {
                    auto parentField = parentDef->getField(varName);
                    if (parentField && !parentField->isStatic()) {
                        auto accessMod = parentField->getAccessModifier();
                        if (accessMod != ast::AccessModifier::PRIVATE) {
                            auto uType = parentField->getUnifiedType();
                            if (uType) {
                                return ::types::TypeConversionUtils::stripNullable(uType->toString());
                            }
                        }
                    }
                    parentClassName = parentDef->getParentClassName();
                    if (parentClassName.empty()) break;
                    parentDef = environment->getClassRegistry()->findClass(parentClassName);
                }
            }
        }

        if (globalRegistry.exists(varName)) {
            return globalRegistry.getClassName(varName);
        }

        return "";
    }

    std::string TypeInferenceEngine::inferFunctionCallClassName(ast::FunctionCallNode* funcCall) const
    {
        // Cache hit: function-call resolution stashes the substituted return
        // type during compilation (while bindings are active) so this lookup
        // works even after bindings are popped.
        if (resolvedFunctionCallTypes)
        {
            auto it = resolvedFunctionCallTypes->find(funcCall);
            if (it != resolvedFunctionCallTypes->end())
            {
                return it->second;
            }
        }

        std::string functionName = funcCall->getFunctionName();
        const auto* funcMetadata = findOverloadMetadata(
            functionName,
            funcCall->getArgumentCount(),
            funcCall->getArguments());

        if (funcMetadata && !funcMetadata->returnType.empty() && !isUnboundGenericReturn(funcMetadata)) {
            if (funcMetadata->returnType != "int" && funcMetadata->returnType != "float" &&
                funcMetadata->returnType != "string" && funcMetadata->returnType != "bool" &&
                funcMetadata->returnType != "void" && funcMetadata->returnType != "object") {
                return ::types::TypeConversionUtils::stripNullable(resolveGenericType(funcMetadata->returnType));
            }
        }

        auto funcDef = environment->findFunction(functionName);
        if (funcDef) {
            std::string returnClassName = funcDef->getReturnClassName();
            if (!returnClassName.empty()) {
                return ::types::TypeConversionUtils::stripNullable(resolveGenericType(returnClassName));
            }
        }
        return "";
    }

    std::string TypeInferenceEngine::inferIndexAccessClassName(ast::nodes::expressions::IndexAccessNode* indexAccess) const
    {
        std::string collectionClassName = inferExpressionClassName(indexAccess->getCollection());

        std::string elementType;

        if (collectionClassName.find("Array<") == 0)
        {
            size_t start = collectionClassName.find('<');
            size_t end = collectionClassName.rfind('>');
            if (start != std::string::npos && end != std::string::npos && end > start)
            {
                elementType = collectionClassName.substr(start + 1, end - start - 1);
            }
        }
        else if (!collectionClassName.empty() && collectionClassName.length() >= 2 &&
                 collectionClassName.substr(collectionClassName.length() - 2) == "[]")
        {
            elementType = collectionClassName.substr(0, collectionClassName.length() - 2);
        }

        if (!elementType.empty())
        {
            if (elementType != "int" && elementType != "float" &&
                elementType != "string" && elementType != "bool" &&
                elementType != "void")
            {
                return elementType;
            }
        }

        return "";
    }

    std::string TypeInferenceEngine::inferMemberAccessClassName(ast::MemberAccessNode* memberAccess) const
    {
        std::string className = inferExpressionClassName(memberAccess->getObject());
        if (!className.empty()) {
            auto classDef = environment->findClass(className);
            if (classDef) {
                std::string memberName = memberAccess->getMemberName();

                auto field = classDef->getField(memberName);
                if (field) {
                    if (field->hasUnifiedType()) {
                        std::string fieldTypeName = ::types::TypeConversionUtils::stripNullable(
                            field->getUnifiedType()->toString());
                        if (fieldTypeName != "int" && fieldTypeName != "float" &&
                            fieldTypeName != "string" && fieldTypeName != "bool" &&
                            fieldTypeName != "void") {
                            return fieldTypeName;
                        }
                    }
                }

                auto method = classDef->getMethod(memberName);
                if (method) {
                    if (method->getUnifiedReturnType()) {
                        std::string returnTypeName = ::types::TypeConversionUtils::stripNullable(
                            method->getUnifiedReturnType()->toString());
                        if (returnTypeName != "int" && returnTypeName != "float" &&
                            returnTypeName != "string" && returnTypeName != "bool" &&
                            returnTypeName != "void") {
                            return returnTypeName;
                        }
                    }
                }
            }
        }
        return "";
    }

    std::string TypeInferenceEngine::inferMethodCallClassName(ast::MethodCallNode* methodCall) const
    {
        std::string className = resolveMethodCallReceiverClassName(methodCall);
        if (className.empty()) {
            return "";
        }

        const auto* funcMetadata = findInstanceMethodMetadata(className, methodCall);

        // Generic methods: substitute call-site type arguments into the return
        // type (handles plain "T" and array forms like "T[]"/"T[][]").
        if (methodCall->hasGenericTypeArguments() && funcMetadata && !funcMetadata->genericTypeParameters.empty())
        {
            const auto& genericTypeArgs = methodCall->getGenericTypeArguments();
            const auto& genericTypeParams = funcMetadata->genericTypeParameters;
            std::string returnType = funcMetadata->returnType;

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
                        return it->second + arrayDimensions;
                    }
                }
                else
                {
                    auto it = substitutions.find(returnType);
                    if (it != substitutions.end())
                    {
                        return it->second;
                    }
                }
            }
        }

        bool genericArityMatches = funcMetadata &&
            (!methodCall->hasGenericTypeArguments() ||
             funcMetadata->genericTypeParameters.size() == methodCall->getGenericTypeArguments().size());

        if (funcMetadata && genericArityMatches && !funcMetadata->returnType.empty()) {
            std::string returnType = applyGenericMethodReturnSubstitutions(
                funcMetadata->returnType,
                methodCall,
                funcMetadata->genericTypeParameters);
            if (isUnresolvedGenericReturnTypeName(returnType)) {
                return "";
            }
            value::ValueType classifiedType = classifyReturnTypeName(returnType);
            std::string resolvedReturnType = ::types::TypeConversionUtils::stripNullable(resolveGenericType(returnType));
            if ((classifiedType == value::ValueType::OBJECT || classifiedType == value::ValueType::ARRAY) &&
                resolvedReturnType != "object") {
                return resolvedReturnType;
            }
        }

        auto methodDef = resolveEnvironmentMethodCall(methodCall);
        if (methodDef) {
            std::string returnType = getMethodDefinitionReturnTypeName(methodDef, methodCall);
            if (isUnresolvedGenericReturnTypeName(returnType)) {
                return "";
            }
            value::ValueType classifiedType = classifyReturnTypeName(returnType);
            std::string resolvedReturnType = ::types::TypeConversionUtils::stripNullable(resolveGenericType(returnType));
            if ((classifiedType == value::ValueType::OBJECT || classifiedType == value::ValueType::ARRAY) &&
                resolvedReturnType != "object") {
                return resolvedReturnType;
            }
        }

        return "";
    }

    std::string TypeInferenceEngine::inferArrayCreationClassName(
        ast::nodes::expressions::ArrayCreationNode* arrCreate) const
    {
        // MYT-282: emit the precise full form ("int[]", "Animal[]"...) so that
        // generic-parameter binding sees a concrete array type instead of the
        // coarse "array" fallback. One pair of brackets per declared dimension.
        std::string elementName = arrCreate->getElementTypeInfo().toString();
        if (elementName.empty()) {
            return "";
        }
        std::string result = std::move(elementName);
        for (size_t i = 0; i < arrCreate->getDimensionCount(); ++i) {
            result += "[]";
        }
        return result;
    }

    std::string TypeInferenceEngine::inferArrayLiteralClassName(
        ast::nodes::expressions::ArrayLiteralNode* arrayLit) const
    {
        if (arrayLit->getElementCount() > 0) {
            const auto& elements = arrayLit->getElements();
            value::ValueType elementType = inferExpressionType(elements[0].get());

            if (elementType == value::ValueType::INT) return "int[]";
            if (elementType == value::ValueType::FLOAT) return "float[]";
            if (elementType == value::ValueType::STRING) return "string[]";
            if (elementType == value::ValueType::BOOL) return "bool[]";
            if (elementType == value::ValueType::OBJECT) {
                std::string elementClassName = inferExpressionClassName(elements[0].get());
                if (!elementClassName.empty()) {
                    return elementClassName + "[]";
                }
            }
        }
        return "int[]";
    }

    std::string TypeInferenceEngine::inferBinaryOpClassName(ast::BinaryOpNode* binOp) const
    {
        auto leftType = inferExpressionType(binOp->getLeft());
        auto rightType = inferExpressionType(binOp->getRight());
        auto op = binOp->getOperator();

        std::string leftClassName;
        if (!tryBinaryOperatorOverload(binOp, leftType, rightType, leftClassName))
        {
            return "";
        }

        bool isBoxTypeOp = (leftClassName == "Int" || leftClassName == "Float" ||
                            leftClassName == "Bool" || leftClassName == "String");
        if (!isBoxTypeOp) {
            return "";
        }

        // equals() returns primitive bool, not a Bool object
        if (op == token::TokenType::EQUALS || op == token::TokenType::NOT_EQUALS)
        {
            return "";
        }

        if (op == token::TokenType::LESS || op == token::TokenType::GREATER ||
            op == token::TokenType::LESS_EQUALS || op == token::TokenType::GREATER_EQUALS)
        {
            return "Bool";
        }

        if (op == token::TokenType::PLUS || op == token::TokenType::MINUS ||
            op == token::TokenType::MULTIPLY || op == token::TokenType::DIVIDE ||
            op == token::TokenType::MODULO)
        {
            return leftClassName;
        }

        return "";
    }

    std::string TypeInferenceEngine::inferAwaitClassName(
        ast::nodes::expressions::AwaitExpression* awaitExpr) const
    {
        auto* innerExpr = awaitExpr->getExpressionPtr();
        if (!innerExpr) {
            return "";
        }

        auto unwrapPromise = [](const std::string& className) -> std::string {
            if (className.find("Promise<") != 0) {
                return className;
            }
            size_t start = className.find('<');
            size_t end = className.rfind('>');
            if (start != std::string::npos && end != std::string::npos && end > start) {
                return className.substr(start + 1, end - start - 1);
            }
            return className;
        };

        if (auto* funcCall = dynamic_cast<ast::FunctionCallNode*>(innerExpr)) {
            return unwrapPromise(inferFunctionCallClassName(funcCall));
        }
        return unwrapPromise(inferExpressionClassName(innerExpr));
    }

    std::string TypeInferenceEngine::inferTernaryClassName(
        ast::nodes::expressions::TernaryExpNode* ternary) const
    {
        std::string trueClassName = inferExpressionClassName(ternary->getTrueExpression());
        std::string falseClassName = inferExpressionClassName(ternary->getFalseExpression());

        if (!trueClassName.empty() && trueClassName == falseClassName) {
            return trueClassName;
        }

        return "";
    }

    std::string TypeInferenceEngine::inferExpressionClassName(ast::ASTNode* node) const
    {
        if (!node) return "";

        if (auto* castExpr = dynamic_cast<ast::CastExpression*>(node)) {
            return inferCastClassName(castExpr);
        }

        if (auto* newNode = dynamic_cast<ast::NewNode*>(node)) {
            return newNode->getClassName();
        }

        if (auto* arrCreate = dynamic_cast<ast::nodes::expressions::ArrayCreationNode*>(node)) {
            return inferArrayCreationClassName(arrCreate);
        }

        if (auto* arrayLit = dynamic_cast<ast::nodes::expressions::ArrayLiteralNode*>(node)) {
            return inferArrayLiteralClassName(arrayLit);
        }

        if (auto* varNode = dynamic_cast<ast::VariableNode*>(node)) {
            return inferVariableClassName(varNode);
        }

        if (auto* funcCall = dynamic_cast<ast::FunctionCallNode*>(node)) {
            return inferFunctionCallClassName(funcCall);
        }

        if (auto* indexAccess = dynamic_cast<ast::nodes::expressions::IndexAccessNode*>(node)) {
            return inferIndexAccessClassName(indexAccess);
        }

        if (auto* memberAccess = dynamic_cast<ast::MemberAccessNode*>(node)) {
            return inferMemberAccessClassName(memberAccess);
        }

        if (auto* methodCall = dynamic_cast<ast::MethodCallNode*>(node)) {
            return inferMethodCallClassName(methodCall);
        }

        if (auto* binOp = dynamic_cast<ast::BinaryOpNode*>(node)) {
            return inferBinaryOpClassName(binOp);
        }

        if (auto* awaitExpr = dynamic_cast<ast::nodes::expressions::AwaitExpression*>(node)) {
            return inferAwaitClassName(awaitExpr);
        }

        if (auto* ternary = dynamic_cast<ast::nodes::expressions::TernaryExpNode*>(node)) {
            return inferTernaryClassName(ternary);
        }

        return "";
    }
}
