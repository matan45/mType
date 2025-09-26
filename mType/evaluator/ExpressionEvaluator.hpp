#pragma once
#include "base/EvaluationContext.hpp"
#include "utils/ValueConverter.hpp"
#include "../token/TokenType.hpp"
#include "../ast/NodeClassesDeclaration.hpp"
#include "../ast/nodes/expressions/NullNode.hpp"
#include "../errors/SourceLocation.hpp"
#include "../parser/TypeParser.hpp"
#include <memory>
#include <optional>

namespace evaluator
{
    using namespace base;
    using namespace utils;
    using namespace value;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::functions;
    using namespace ast::nodes::classes;
    using namespace ast::nodes::statements;
    using namespace token;

    /**
     * @brief Refactored Expression Evaluator following SOLID principles
     * - Single Responsibility: Only handles expression evaluation
     * - Open/Closed: Extensible through composition
     * - Liskov Substitution: Implements IExpressionEvaluator interface
     * - Interface Segregation: Uses specialized interfaces
     * - Dependency Inversion: Depends on abstractions (EvaluationContext)
     */
    class ExpressionEvaluator
    {
    private:
        std::shared_ptr<EvaluationContext> context;

        // Forward declarations for circular dependency resolution
        class StatementEvaluator* stmtEvaluator;
        class ObjectEvaluator* objEvaluator;

    public:
        explicit ExpressionEvaluator(std::shared_ptr<EvaluationContext> ctx);
        ~ExpressionEvaluator() = default;

        // Main interface methods
        Value evaluate(ASTNode* node);
        bool canHandle(ASTNode* node) const;

        // Type conversion methods
        bool isTruthy(const Value& value) const;
        std::string toString(const Value& value) const;
        float toFloat(const Value& value) const;
        int toInt(const Value& value) const;

        // Expression evaluation methods (now private implementation details)
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
        Value evaluateAssignmentExpression(AssignmentNode* node);
        Value evaluateArrayCreationNode(ArrayCreationNode* node);
        Value evaluateArrayLiteralNode(ArrayLiteralNode* node);
        Value evaluateIndexAccessNode(IndexAccessNode* node);
        Value evaluateLambdaNode(LambdaNode* node);

        // Helper method to get default value for type
        Value getDefaultValueForType(const ::parser::TypeInfo& elementType);

        // Dependency injection for cross-evaluator communication
        void setStatementEvaluator(StatementEvaluator* evaluator);
        void setObjectEvaluator(ObjectEvaluator* evaluator);

    private:
        // Helper methods for binary operations
        Value evaluateArithmetic(const Value& left, const Value& right, TokenType op);
        Value evaluateComparison(const Value& left, const Value& right, TokenType op);
        Value evaluateLogical(const Value& left, const Value& right, TokenType op);
        Value evaluateStringOperation(const Value& left, const Value& right, TokenType op);

        // Node type checking
        bool isExpressionNode(ASTNode* node) const;

        // Function return type validation
        void validateFunctionReturnType(ValueType expectedType, const Value& returnValue,
                                        const std::string& functionName,
                                        const SourceLocation& location);

        // Multi-dimensional array access helpers
        std::optional<Value> extractMultiDimensionalAccess(IndexAccessNode* node, std::vector<size_t>& indices);
        Value evaluateDirectMultiDimensionalAccess(const Value& baseArray, const std::vector<size_t>& indices, const SourceLocation& location);

        // Type validation helpers
        std::string getTypeNameForError(ValueType type) const;
        bool validateObjectTypeCompatibility(const Value& expected, const Value& actual) const;
        std::string getObjectClassName(const Value& objectValue) const;
    };
}
