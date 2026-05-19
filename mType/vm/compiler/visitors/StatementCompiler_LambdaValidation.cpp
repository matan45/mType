#include "StatementCompiler.hpp"
#include <cstddef>
#include "../../../errors/TypeException.hpp"
#include "../../../ast/nodes/expressions/LambdaNode.hpp"

namespace vm::compiler::visitors
{
    std::vector<ast::nodes::functions::ReturnNode*> StatementCompiler::collectReturnStatements(ast::ASTNode* node)
    {
        std::vector<ast::nodes::functions::ReturnNode*> returns;

        if (!node)
        {
            return returns;
        }

        if (auto* returnNode = dynamic_cast<ast::nodes::functions::ReturnNode*>(node))
        {
            returns.push_back(returnNode);
            return returns;
        }

        if (auto* blockNode = dynamic_cast<ast::nodes::statements::BlockNode*>(node))
        {
            for (const auto& stmt : blockNode->getStatements())
            {
                auto childReturns = collectReturnStatements(stmt.get());
                returns.insert(returns.end(), childReturns.begin(), childReturns.end());
            }
        }
        else if (auto* ifNode = dynamic_cast<ast::nodes::statements::IfNode*>(node))
        {
            auto thenReturns = collectReturnStatements(ifNode->getThenStatement());
            returns.insert(returns.end(), thenReturns.begin(), thenReturns.end());

            if (ifNode->getElseStatement())
            {
                auto elseReturns = collectReturnStatements(ifNode->getElseStatement());
                returns.insert(returns.end(), elseReturns.begin(), elseReturns.end());
            }
        }
        else if (auto* whileNode = dynamic_cast<ast::nodes::statements::WhileNode*>(node))
        {
            auto bodyReturns = collectReturnStatements(whileNode->getBody());
            returns.insert(returns.end(), bodyReturns.begin(), bodyReturns.end());
        }
        else if (auto* forNode = dynamic_cast<ast::nodes::statements::ForNode*>(node))
        {
            auto bodyReturns = collectReturnStatements(forNode->getBody());
            returns.insert(returns.end(), bodyReturns.begin(), bodyReturns.end());
        }
        else if (auto* tryNode = dynamic_cast<ast::nodes::statements::TryNode*>(node))
        {
            auto tryReturns = collectReturnStatements(tryNode->getTryBlock());
            returns.insert(returns.end(), tryReturns.begin(), tryReturns.end());

            for (const auto& catchBlock : tryNode->getCatchBlocks())
            {
                auto catchReturns = collectReturnStatements(catchBlock->getBody());
                returns.insert(returns.end(), catchReturns.begin(), catchReturns.end());
            }

            if (tryNode->getFinallyBlock())
            {
                auto finallyReturns = collectReturnStatements(tryNode->getFinallyBlock());
                returns.insert(returns.end(), finallyReturns.begin(), finallyReturns.end());
            }
        }

        return returns;
    }

    void StatementCompiler::validateLambdaAssignment(ast::AssignmentNode* node, bool /*isReassignment*/,
                                                     const std::string& /*existingClassName*/)
    {
        auto* value = node->getValue();
        if (!value || !dynamic_cast<ast::LambdaNode*>(value))
        {
            return;
        }

        auto* lambdaNode = dynamic_cast<ast::LambdaNode*>(value);
        value::ValueType varType = node->getVariableType();

        // Lambda reassignment checking is disabled because we cannot distinguish
        // between first assignment after declaration and lambda-to-lambda
        // reassignment at compile time without runtime value information.

        if (varType == value::ValueType::OBJECT && !node->getClassName().empty())
        {
            auto interfaceDef = ctx.env->findInterface(node->getClassName());
            if (interfaceDef && !interfaceDef->isFunctionalInterface())
            {
                auto methodSignatures = interfaceDef->getMethodSignatures();
                throw errors::TypeException(
                    "Cannot assign lambda to non-functional interface '" + node->getClassName() + "'. " +
                    "Lambdas can only be assigned to interfaces with exactly one method. " +
                    "Interface '" + node->getClassName() + "' has " + std::to_string(methodSignatures.size()) +
                    " methods. " +
                    "Consider using a functional interface (single method) or implement the interface explicitly.",
                    node->getLocation()
                );
            }

            if (interfaceDef && interfaceDef->isFunctionalInterface())
            {
                auto methodSignatures = interfaceDef->getMethodSignatures();
                if (!methodSignatures.empty())
                {
                    const auto& methodSig = methodSignatures[0];

                    size_t lambdaParamCount = lambdaNode->getParameters().size();
                    size_t interfaceParamCount = methodSig.parameters.size();
                    if (lambdaParamCount != interfaceParamCount)
                    {
                        throw errors::TypeException(
                            "Lambda parameter count mismatch for interface '" + node->getClassName() + "'. " +
                            "Lambda has " + std::to_string(lambdaParamCount) + " parameter(s) but " +
                            "interface method '" + methodSig.name + "' expects " + std::to_string(interfaceParamCount) + " parameter(s).",
                            node->getLocation()
                        );
                    }

                    // For async interface methods returning Promise<T>, the lambda body
                    // returns T directly (runtime wraps it). Unwrap for comparison.
                    auto effectiveReturnType = methodSig.returnType;
                    if (effectiveReturnType && effectiveReturnType->getName() == "Promise"
                        && effectiveReturnType->isParameterized()) {
                        const auto& typeArgs = effectiveReturnType->getTypeArguments();
                        if (!typeArgs.empty()) {
                            effectiveReturnType = typeArgs[0];
                        }
                    }

                    // Skip validation for generic type parameters (T, R, …) — validated at runtime.
                    if (effectiveReturnType && !effectiveReturnType->isGenericParameter())
                    {
                        auto* body = lambdaNode->getBody();
                        std::string expectedTypeStr = effectiveReturnType->toString();

                        if (lambdaNode->isExpressionLambda())
                        {
                            if (body)
                            {
                                value::ValueType lambdaReturnType = ctx.typeInference.inferExpressionType(body);
                                std::string lambdaReturnClassName = ctx.typeInference.inferExpressionClassName(body);

                                // Skip validation for expressions using parameters (like 'x * 2')
                                // — parameters aren't in scope during this validation phase.
                                if (lambdaReturnType != value::ValueType::VOID)
                                {
                                    std::string actualTypeStr;
                                    if (lambdaReturnType == value::ValueType::OBJECT && !lambdaReturnClassName.empty())
                                    {
                                        actualTypeStr = lambdaReturnClassName;
                                    }
                                    else if (lambdaReturnType == value::ValueType::INT) actualTypeStr = "int";
                                    else if (lambdaReturnType == value::ValueType::STRING) actualTypeStr = "string";
                                    else if (lambdaReturnType == value::ValueType::BOOL) actualTypeStr = "bool";
                                    else if (lambdaReturnType == value::ValueType::FLOAT) actualTypeStr = "float";
                                    else actualTypeStr = "unknown";

                                    if (expectedTypeStr != actualTypeStr)
                                    {
                                        throw errors::TypeException(
                                            "Lambda return type mismatch for interface '" + node->getClassName() + "'. " +
                                            "Lambda returns '" + actualTypeStr + "' but " +
                                            "interface method '" + methodSig.name + "' expects '" + expectedTypeStr + "'.",
                                            node->getLocation()
                                        );
                                    }
                                }
                            }
                        }
                        else
                        {
                            auto returnStatements = collectReturnStatements(body);

                            if (expectedTypeStr != "void" && returnStatements.empty())
                            {
                                std::string errorMsg = "Lambda is missing return statement. " +
                                    std::string("Interface method '") + methodSig.name + "' expects return type '" + expectedTypeStr + "' " +
                                    "but lambda body has no return statement.";
                                throw errors::TypeException(errorMsg, node->getLocation());
                            }

                            for (auto* returnNode : returnStatements)
                            {
                                auto* returnExpr = returnNode->getReturnValue();
                                if (returnExpr)
                                {
                                    value::ValueType returnType = ctx.typeInference.inferExpressionType(returnExpr);
                                    std::string returnClassName = ctx.typeInference.inferExpressionClassName(returnExpr);

                                    std::string actualTypeStr;
                                    if (returnType == value::ValueType::OBJECT && !returnClassName.empty())
                                    {
                                        actualTypeStr = returnClassName;
                                    }
                                    else if (returnType == value::ValueType::INT) actualTypeStr = "int";
                                    else if (returnType == value::ValueType::STRING) actualTypeStr = "string";
                                    else if (returnType == value::ValueType::BOOL) actualTypeStr = "bool";
                                    else if (returnType == value::ValueType::FLOAT) actualTypeStr = "float";
                                    else if (returnType == value::ValueType::VOID)
                                    {
                                        // Skip void returns from parameter references.
                                        continue;
                                    }
                                    else actualTypeStr = "unknown";

                                    if (expectedTypeStr != actualTypeStr)
                                    {
                                        throw errors::TypeException(
                                            "Lambda return type mismatch for interface '" + node->getClassName() + "'. " +
                                            "Lambda returns '" + actualTypeStr + "' but " +
                                            "interface method '" + methodSig.name + "' expects '" + expectedTypeStr + "'.",
                                            returnNode->getLocation()
                                        );
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
