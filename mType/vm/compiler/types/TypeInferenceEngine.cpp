#include "TypeInferenceEngine.hpp"
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
#include "../../../ast/nodes/classes/NewNode.hpp"
#include "../../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../token/TokenType.hpp"

namespace vm::compiler::types
{
    TypeInferenceEngine::TypeInferenceEngine(
        const bytecode::BytecodeProgram& program,
        std::shared_ptr<environment::Environment> environment,
        const variables::VariableTracker& variableTracker,
        const variables::GlobalVariableRegistry& globalRegistry
    )
        : program(program)
        , environment(environment)
        , variableTracker(variableTracker)
        , globalRegistry(globalRegistry)
    {
    }

    value::ValueType TypeInferenceEngine::inferExpressionType(ast::ASTNode* node) const
    {
        if (!node) return value::ValueType::VOID;

        // Literal types
        if (dynamic_cast<ast::IntegerNode*>(node)) return value::ValueType::INT;
        if (dynamic_cast<ast::FloatNode*>(node)) return value::ValueType::FLOAT;
        if (dynamic_cast<ast::StringNode*>(node)) return value::ValueType::STRING;
        if (dynamic_cast<ast::BoolNode*>(node)) return value::ValueType::BOOL;
        if (dynamic_cast<ast::NullNode*>(node)) return value::ValueType::OBJECT;
        if (dynamic_cast<ast::NewNode*>(node)) return value::ValueType::OBJECT;
        if (dynamic_cast<ast::LambdaNode*>(node)) return value::ValueType::OBJECT;

        // Variable references
        if (auto* varNode = dynamic_cast<ast::VariableNode*>(node)) {
            std::string varName = varNode->getName();

            // Check locals
            const auto& locals = variableTracker.getLocals();
            for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
                if (it->name == varName) {
                    return it->type;
                }
            }

            // Check globals
            if (globalRegistry.exists(varName)) {
                return globalRegistry.getType(varName);
            }

            return value::ValueType::VOID;
        }

        // Function calls
        if (auto* funcCall = dynamic_cast<ast::FunctionCallNode*>(node)) {
            const auto* funcMetadata = program.getFunction(funcCall->getFunctionName());
            if (funcMetadata && !funcMetadata->returnType.empty()) {
                if (funcMetadata->returnType == "int") return value::ValueType::INT;
                if (funcMetadata->returnType == "float") return value::ValueType::FLOAT;
                if (funcMetadata->returnType == "string") return value::ValueType::STRING;
                if (funcMetadata->returnType == "bool") return value::ValueType::BOOL;
                return value::ValueType::OBJECT;
            }
        }

        // Unary operations
        if (auto* unaryOp = dynamic_cast<ast::UnaryOpNode*>(node)) {
            auto operandType = inferExpressionType(unaryOp->getOperand());
            auto op = unaryOp->getOperator();

            // Unary minus and plus preserve numeric type
            if (op == token::TokenType::MINUS || op == token::TokenType::PLUS) {
                if (operandType == value::ValueType::INT || operandType == value::ValueType::FLOAT) {
                    return operandType;
                }
            }

            // Logical NOT returns bool
            if (op == token::TokenType::NOT) {
                return value::ValueType::BOOL;
            }

            return operandType;
        }

        // Cast expressions
        if (auto* castExpr = dynamic_cast<ast::CastExpression*>(node)) {
            const auto* targetType = castExpr->getTargetType();
            if (targetType) {
                std::string targetTypeName = targetType->toString();
                if (targetTypeName == "int") return value::ValueType::INT;
                if (targetTypeName == "float") return value::ValueType::FLOAT;
                if (targetTypeName == "string") return value::ValueType::STRING;
                if (targetTypeName == "bool") return value::ValueType::BOOL;
                return value::ValueType::OBJECT;
            }
        }

        // Binary operations
        if (auto* binOp = dynamic_cast<ast::BinaryOpNode*>(node)) {
            auto leftType = inferExpressionType(binOp->getLeft());
            auto rightType = inferExpressionType(binOp->getRight());
            auto op = binOp->getOperator();

            // String concatenation with + always results in string
            if (op == token::TokenType::PLUS &&
                (leftType == value::ValueType::STRING || rightType == value::ValueType::STRING)) {
                return value::ValueType::STRING;
            }

            // Arithmetic operations on int/float
            if (op == token::TokenType::PLUS || op == token::TokenType::MINUS ||
                op == token::TokenType::MULTIPLY || op == token::TokenType::DIVIDE) {
                if (leftType == value::ValueType::FLOAT || rightType == value::ValueType::FLOAT) {
                    return value::ValueType::FLOAT;
                }
                if (leftType == value::ValueType::INT && rightType == value::ValueType::INT) {
                    return value::ValueType::INT;
                }
            }

            // Comparison operations return bool
            if (op == token::TokenType::EQUALS || op == token::TokenType::NOT_EQUALS ||
                op == token::TokenType::LESS || op == token::TokenType::GREATER ||
                op == token::TokenType::LESS_EQUALS || op == token::TokenType::GREATER_EQUALS) {
                return value::ValueType::BOOL;
            }

            // Logical operations return bool
            if (op == token::TokenType::AND || op == token::TokenType::OR) {
                return value::ValueType::BOOL;
            }
        }

        return value::ValueType::VOID;
    }

    std::string TypeInferenceEngine::inferExpressionClassName(ast::ASTNode* node) const
    {
        if (!node) return "";

        // Cast expressions
        if (auto* castExpr = dynamic_cast<ast::CastExpression*>(node)) {
            const auto* targetType = castExpr->getTargetType();
            if (targetType) {
                std::string targetTypeName = targetType->toString();
                // Only return class name if it's not a primitive type
                if (targetTypeName != "int" && targetTypeName != "float" &&
                    targetTypeName != "string" && targetTypeName != "bool" &&
                    targetTypeName != "void") {
                    return targetTypeName;
                }
            }
            return "";
        }

        // NewNode
        if (auto* newNode = dynamic_cast<ast::NewNode*>(node)) {
            return newNode->getClassName();
        }

        // Variable references
        if (auto* varNode = dynamic_cast<ast::VariableNode*>(node)) {
            std::string varName = varNode->getName();

            // Check locals
            const auto& locals = variableTracker.getLocals();
            for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
                if (it->name == varName) {
                    return it->className;
                }
            }

            // Check globals
            if (globalRegistry.exists(varName)) {
                return globalRegistry.getClassName(varName);
            }

            return "";
        }

        // Function calls
        if (auto* funcCall = dynamic_cast<ast::FunctionCallNode*>(node)) {
            std::string functionName = funcCall->getFunctionName();
            const auto* funcMetadata = program.getFunction(functionName);

            // If this is a generic function call, resolve the return type using the provided type arguments
            if (funcCall->hasGenericTypeArguments() && funcMetadata && !funcMetadata->genericTypeParameters.empty())
            {
                const auto& genericTypeArgs = funcCall->getGenericTypeArguments();
                const auto& genericTypeParams = funcMetadata->genericTypeParameters;
                std::string returnType = funcMetadata->returnType;

                // Build temporary bindings for this function call
                for (size_t i = 0; i < genericTypeParams.size() && i < genericTypeArgs.size(); ++i)
                {
                    if (returnType == genericTypeParams[i])
                    {
                        // The return type is a generic parameter, substitute it
                        return genericTypeArgs[i];
                    }
                }
            }

            if (funcMetadata && !funcMetadata->returnType.empty()) {
                // If return type is not a primitive, it's a class name
                if (funcMetadata->returnType != "int" && funcMetadata->returnType != "float" &&
                    funcMetadata->returnType != "string" && funcMetadata->returnType != "bool" &&
                    funcMetadata->returnType != "void" && funcMetadata->returnType != "object") {
                    // Resolve generic type if applicable (from context stack)
                    return resolveGenericType(funcMetadata->returnType);
                }
            }

            // Also check in the environment for functions
            auto funcDef = environment->findFunction(functionName);
            if (funcDef) {
                std::string returnClassName = funcDef->getReturnClassName();
                if (!returnClassName.empty()) {
                    // Resolve generic type if applicable
                    return resolveGenericType(returnClassName);
                }
            }
            return "";
        }

        return "";
    }

    void TypeInferenceEngine::setGenericTypeBindingsStack(
        const std::vector<std::unordered_map<std::string, std::string>>* stack)
    {
        genericTypeBindingsStack = stack;
    }

    std::string TypeInferenceEngine::resolveGenericType(const std::string& typeName) const
    {
        if (!genericTypeBindingsStack || genericTypeBindingsStack->empty())
        {
            return typeName;
        }

        // Check from most recent to oldest binding context
        for (auto it = genericTypeBindingsStack->rbegin(); it != genericTypeBindingsStack->rend(); ++it)
        {
            auto found = it->find(typeName);
            if (found != it->end())
            {
                return found->second;
            }
        }

        // Not a generic type parameter, return as-is
        return typeName;
    }
}
