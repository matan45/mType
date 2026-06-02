#include "TypeInferenceEngine.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../../ast/nodes/expressions/CastExpression.hpp"
#include "../../../ast/nodes/expressions/LambdaNode.hpp"
#include "../../../ast/nodes/expressions/AwaitExpression.hpp"
#include "../../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../../../ast/nodes/classes/NewNode.hpp"
#include "../../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../../ast/nodes/classes/SuperMemberAccessNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/functions/FunctionCallNode.hpp"

namespace vm::compiler::types
{
    bool TypeInferenceEngine::inferVariableNullable(ast::VariableNode* varNode) const
    {
        const std::string& varName = varNode->getName();

        if (varName == "this")
        {
            return false;
        }

        if (nullNarrowingTracker && nullNarrowingTracker->isNarrowedNonNull(varName))
        {
            return false;
        }

        if (variableTracker.existsInFunction(varName))
        {
            return variableTracker.getLocalNullableByName(varName);
        }

        if (globalRegistry.exists(varName))
        {
            return globalRegistry.isNullable(varName);
        }

        return false;
    }

    bool TypeInferenceEngine::inferMethodCallNullable(ast::MethodCallNode* methodCall) const
    {
        // MYT-374: a safe-navigation call (obj?.method()) can short-circuit to
        // null, so its result is always nullable regardless of the method's
        // declared return type.
        if (methodCall->getIsSafe())
        {
            return true;
        }

        std::string className = inferExpressionClassName(methodCall->getObject());
        if (!className.empty())
        {
            std::string qualifiedName = className + "::" + methodCall->getMethodName();
            const auto* funcMeta = program.getFunction(qualifiedName);

            // Overload prefix-match: conservative — if any overload returns
            // nullable, treat the whole call as potentially-nullable.
            if (!funcMeta)
            {
                const auto& allFunctions = program.getFunctions();
                std::string prefix = qualifiedName + "/";
                for (const auto& [name, metadata] : allFunctions)
                {
                    if (name.find(prefix) == 0 || name == qualifiedName)
                    {
                        if (!funcMeta)
                        {
                            funcMeta = &metadata;
                        }
                        else if (!::types::TypeConversionUtils::isNullableType(funcMeta->returnType)
                                 && ::types::TypeConversionUtils::isNullableType(metadata.returnType))
                        {
                            funcMeta = &metadata;
                        }
                    }
                }
            }

            if (funcMeta && !funcMeta->returnType.empty())
            {
                return ::types::TypeConversionUtils::isNullableType(funcMeta->returnType);
            }
        }

        if (!className.empty())
        {
            auto classDef = environment->findClass(className);
            if (classDef)
            {
                auto method = classDef->getMethod(methodCall->getMethodName());
                if (method && method->getUnifiedReturnType())
                {
                    return method->getUnifiedReturnType()->isNullable();
                }
            }
        }

        return false;
    }

    bool TypeInferenceEngine::inferMemberAccessNullable(ast::MemberAccessNode* memberAccess) const
    {
        // MYT-374: a safe-navigation access (obj?.field) can short-circuit to
        // null, so its result is always nullable regardless of the field's
        // declared type.
        if (memberAccess->getIsSafe())
        {
            return true;
        }

        std::string className = inferExpressionClassName(memberAccess->getObject());
        if (!className.empty())
        {
            auto classDef = environment->findClass(className);
            if (classDef)
            {
                auto field = classDef->getField(memberAccess->getMemberName());
                if (field && field->hasUnifiedType())
                {
                    return field->getUnifiedType()->isNullable();
                }
            }
        }
        return false;
    }

    bool TypeInferenceEngine::inferExpressionNullable(ast::ASTNode* node) const
    {
        if (!node) return false;

        if (dynamic_cast<ast::NullNode*>(node))
        {
            return true;
        }

        if (dynamic_cast<ast::IntegerNode*>(node) ||
            dynamic_cast<ast::FloatNode*>(node) ||
            dynamic_cast<ast::StringNode*>(node) ||
            dynamic_cast<ast::BoolNode*>(node))
        {
            return false;
        }

        if (dynamic_cast<ast::NewNode*>(node) ||
            dynamic_cast<ast::LambdaNode*>(node) ||
            dynamic_cast<ast::ArrayLiteralNode*>(node) ||
            dynamic_cast<ast::SuperMemberAccessNode*>(node))
        {
            return false;
        }

        if (dynamic_cast<ast::BinaryOpNode*>(node) ||
            dynamic_cast<ast::UnaryOpNode*>(node))
        {
            return false;
        }

        if (auto* varNode = dynamic_cast<ast::VariableNode*>(node))
        {
            return inferVariableNullable(varNode);
        }

        // MYT-220: a cast cannot launder a literal null into a non-nullable
        // value; the literal-null path must always report nullable.
        if (auto* castExpr = dynamic_cast<ast::CastExpression*>(node))
        {
            if (isEffectivelyNullLiteral(castExpr->getExpression()))
            {
                return true;
            }
            if (castExpr->getTargetType())
            {
                return castExpr->getTargetType()->isNullable();
            }
            return false;
        }

        if (auto* funcCall = dynamic_cast<ast::FunctionCallNode*>(node))
        {
            const std::string& funcName = funcCall->getFunctionName();
            const auto* funcMeta = program.getFunction(funcName);
            if (funcMeta && !funcMeta->returnType.empty())
            {
                return ::types::TypeConversionUtils::isNullableType(funcMeta->returnType);
            }
            return false;
        }

        if (auto* methodCall = dynamic_cast<ast::MethodCallNode*>(node))
        {
            return inferMethodCallNullable(methodCall);
        }

        if (auto* memberAccess = dynamic_cast<ast::MemberAccessNode*>(node))
        {
            return inferMemberAccessNullable(memberAccess);
        }

        if (auto* awaitExpr = dynamic_cast<ast::nodes::expressions::AwaitExpression*>(node))
        {
            auto* innerExpr = awaitExpr->getExpressionPtr();
            if (innerExpr)
            {
                return inferExpressionNullable(innerExpr);
            }
        }

        if (dynamic_cast<ast::nodes::expressions::IndexAccessNode*>(node))
        {
            return false;
        }

        return false;
    }
}
