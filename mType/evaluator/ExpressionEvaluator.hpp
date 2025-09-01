#pragma once
#include "../value/ValueType.hpp"
#include "../token/TokenType.hpp"
#include "../ast/NodeClassesDeclaration.hpp"

namespace evaluator
{
    using namespace value;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::functions;
    using namespace ast::nodes::classes;
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

        //TODO move to utils
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
