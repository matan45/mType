#pragma once
#include "../value/ValueType.hpp"
#include "../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../ast/nodes/expressions/VariableNode.hpp"
#include "../ast/nodes/expressions/IntegerNode.hpp"
#include "../ast/nodes/expressions/FloatNode.hpp"
#include "../ast/nodes/expressions/StringNode.hpp"
#include "../ast/nodes/expressions/BoolNode.hpp"
#include "../ast/nodes/expressions/NullNode.hpp"
#include "../ast/nodes/functions/FunctionCallNode.hpp"
#include "../ast/nodes/classes/MemberAccessNode.hpp"
#include "../ast/nodes/classes/MethodCallNode.hpp"
#include "../ast/nodes/classes/NewNode.hpp"
#include "../ast/nodes/namespaces/QualifiedNameNode.hpp"
#include "../token/TokenType.hpp"
#include <memory>

namespace evaluator
{
    using namespace value;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::functions;
    using namespace ast::nodes::classes;
    using namespace ast::nodes::namespaces;
    using namespace token;
    
    class Evaluator;
    
    class ExpressionEvaluator
    {
    private:
        Evaluator* mainEvaluator;
        
    public:
        explicit ExpressionEvaluator(Evaluator* evaluator);
        ~ExpressionEvaluator() = default;
        
        // Expression evaluation methods
        Value evaluateIntegerNode(IntegerNode* node);
        Value evaluateFloatNode(FloatNode* node);
        Value evaluateStringNode(StringNode* node);
        Value evaluateBoolNode(BoolNode* node);
        Value evaluateNullNode(NullNode* node);
        Value evaluateVariableNode(VariableNode* node);
        Value evaluateBinaryExpNode(BinaryExpNode* node);
        Value evaluateTernaryExpNode(TernaryExpNode* node);
        Value evaluateUnaryExpNode(UnaryExpNode* node);
        Value evaluateFunctionCallNode(FunctionCallNode* node);
        Value evaluateMemberAccessNode(MemberAccessNode* node);
        Value evaluateMethodCallNode(MethodCallNode* node);
        Value evaluateNewNode(NewNode* node);
        Value evaluateQualifiedNameNode(QualifiedNameNode* node);
        
        // Helper methods for binary operations
        Value evaluateArithmetic(const Value& left, const Value& right, TokenType op);
        Value evaluateComparison(const Value& left, const Value& right, TokenType op);
        Value evaluateLogical(const Value& left, const Value& right, TokenType op);
        Value evaluateStringOperation(const Value& left, const Value& right, TokenType op);
        
        // Type coercion helpers
        bool isTruthy(const Value& value);
        float toFloat(const Value& value);
        int toInt(const Value& value);
        std::string toString(const Value& value);
    };
}
