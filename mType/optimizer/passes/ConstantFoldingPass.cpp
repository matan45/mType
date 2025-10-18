#include "ConstantFoldingPass.hpp"
#include "constant_folding/ConstantEvaluator.hpp"
#include "../OptimizationResult.hpp"
#include "../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../ast/nodes/expressions/FloatNode.hpp"
#include "../../ast/nodes/expressions/BoolNode.hpp"
#include "../../ast/nodes/expressions/StringNode.hpp"
#include "../../ast/nodes/expressions/NullNode.hpp"
#include "../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../ast/nodes/expressions/CastExpression.hpp"
#include "../../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../../ast/nodes/functions/ReturnNode.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../ast/nodes/statements/IfNode.hpp"
#include "../../ast/nodes/statements/WhileNode.hpp"
#include "../../ast/nodes/statements/ForNode.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/statements/BlockNode.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../ast/nodes/classes/MethodNode.hpp"
#include "../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../value/StringPool.hpp"
#include <iostream>

namespace optimizer::passes {

// Debug flag - set to true to enable debug logging
constexpr bool CF_DEBUG = false;

using namespace ast;
using namespace constant_folding;
using value::Value;
using value::InternedString;
using token::TokenType;
using errors::SourceLocation;

// ==================== CFTransformer Implementation ====================

ConstantFoldingPass::CFTransformer::CFTransformer(
    base::OptimizationContext* ctx,
    size_t& count
) : ASTTransformer(ctx), foldedExpressionsCount(count) {
}

bool ConstantFoldingPass::CFTransformer::isLiteralNode(ast::ASTNode* node) const {
    if (!node) return false;

    return dynamic_cast<ast::nodes::expressions::IntegerNode*>(node) != nullptr ||
           dynamic_cast<ast::nodes::expressions::FloatNode*>(node) != nullptr ||
           dynamic_cast<ast::nodes::expressions::BoolNode*>(node) != nullptr ||
           dynamic_cast<ast::nodes::expressions::StringNode*>(node) != nullptr ||
           dynamic_cast<ast::nodes::expressions::NullNode*>(node) != nullptr;
}

std::optional<Value> ConstantFoldingPass::CFTransformer::extractLiteralValue(ast::ASTNode* node) {
    if (!node) return std::nullopt;

    if (auto* intNode = dynamic_cast<ast::nodes::expressions::IntegerNode*>(node)) {
        return Value(intNode->getValue());
    }
    if (auto* floatNode = dynamic_cast<ast::nodes::expressions::FloatNode*>(node)) {
        return Value(floatNode->getValue());
    }
    if (auto* boolNode = dynamic_cast<ast::nodes::expressions::BoolNode*>(node)) {
        return Value(boolNode->getValue());
    }
    if (auto* strNode = dynamic_cast<ast::nodes::expressions::StringNode*>(node)) {
        return Value(strNode->getValue());
    }
    if (dynamic_cast<ast::nodes::expressions::NullNode*>(node)) {
        return Value(std::monostate{});
    }

    return std::nullopt;
}

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::createLiteralNode(
    const Value& value,
    const SourceLocation& loc
) {
    if (std::holds_alternative<int>(value)) {
        return std::make_unique<ast::nodes::expressions::IntegerNode>(std::get<int>(value), loc);
    }
    if (std::holds_alternative<float>(value)) {
        return std::make_unique<ast::nodes::expressions::FloatNode>(std::get<float>(value), loc);
    }
    if (std::holds_alternative<bool>(value)) {
        return std::make_unique<ast::nodes::expressions::BoolNode>(std::get<bool>(value), loc);
    }
    if (std::holds_alternative<std::string>(value)) {
        return std::make_unique<ast::nodes::expressions::StringNode>(std::get<std::string>(value), loc);
    }
    if (std::holds_alternative<InternedString>(value)) {
        return std::make_unique<ast::nodes::expressions::StringNode>(std::get<InternedString>(value).getString(), loc);
    }
    if (std::holds_alternative<std::monostate>(value) ||
        std::holds_alternative<nullptr_t>(value)) {
        return std::make_unique<ast::nodes::expressions::NullNode>(loc);
    }

    return nullptr;
}

// ==================== Binary Operation Folding ====================

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::tryFoldBinaryOp(
    BinaryOpNode* node
) {
    if (CF_DEBUG) {
        std::cout << "[CF] tryFoldBinaryOp: Entered for binary operation\n";
    }

    // First, recursively transform children (bottom-up)
    auto leftChild = transformChild(node->getLeft());
    auto rightChild = transformChild(node->getRight());

    // Extract constant values from children
    auto leftValue = extractLiteralValue(leftChild.get());
    auto rightValue = extractLiteralValue(rightChild.get());

    if (CF_DEBUG) {
        std::cout << "[CF] tryFoldBinaryOp: leftValue=" << (leftValue.has_value() ? "YES" : "NO")
                  << " rightValue=" << (rightValue.has_value() ? "YES" : "NO") << "\n";
    }

    // If both operands are literals, try to fold
    if (leftValue && rightValue) {
        if (CF_DEBUG) {
            std::cout << "[CF] Found foldable binary operation\n";
        }

        auto result = ConstantEvaluator::evaluateBinaryOp(
            *leftValue,
            *rightValue,
            node->getOperator()
        );

        if (result) {
            auto foldedNode = createLiteralNode(*result, node->getLocation());
            if (foldedNode) {
                foldedExpressionsCount++;
                modified = true;

                if (CF_DEBUG) {
                    std::cout << "[CF] ✓ Folded binary expression → ";
                    if (std::holds_alternative<int>(*result)) {
                        std::cout << std::get<int>(*result);
                    } else if (std::holds_alternative<float>(*result)) {
                        std::cout << std::get<float>(*result);
                    } else if (std::holds_alternative<bool>(*result)) {
                        std::cout << (std::get<bool>(*result) ? "true" : "false");
                    }
                    std::cout << "\n";
                }

                return foldedNode;
            }
        }
    }

    // Couldn't fold - reconstruct with transformed children
    auto newNode = std::make_unique<ast::nodes::expressions::BinaryExpNode>(
        std::move(leftChild),
        node->getOperator(),
        std::move(rightChild),
        node->getLocation()
    );
    return newNode;
}

// ==================== Unary Operation Folding ====================

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::tryFoldUnaryOp(
    UnaryOpNode* node
) {
    // Don't fold increment/decrement operations (they have side effects)
    if (node->getOperator() == TokenType::INCREMENT ||
        node->getOperator() == TokenType::DECREMENT) {
        // Just recursively transform the operand
        auto operand = transformChild(node->getOperand());
        auto newNode = std::make_unique<ast::nodes::expressions::UnaryExpNode>(
            node->getOperator(),
            std::move(operand),
            node->getPosition(),
            node->getLocation()
        );
        return newNode;
    }

    // Transform child (bottom-up)
    auto operandChild = transformChild(node->getOperand());

    // Extract constant value
    auto operandValue = extractLiteralValue(operandChild.get());

    // If operand is a literal, try to fold
    if (operandValue) {
        auto result = ConstantEvaluator::evaluateUnaryOp(
            *operandValue,
            node->getOperator()
        );

        if (result) {
            auto foldedNode = createLiteralNode(*result, node->getLocation());
            if (foldedNode) {
                foldedExpressionsCount++;
                modified = true;

                if (CF_DEBUG) {
                    std::cout << "[CF] ✓ Folded unary expression\n";
                }

                return foldedNode;
            }
        }
    }

    // Couldn't fold - reconstruct with transformed child
    auto newNode = std::make_unique<ast::nodes::expressions::UnaryExpNode>(
        node->getOperator(),
        std::move(operandChild),
        node->getPosition(),
        node->getLocation()
    );
    return newNode;
}

// ==================== Ternary Operation Folding ====================

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::tryFoldTernary(
    TernaryOpNode* node
) {
    // Transform condition (bottom-up)
    auto condition = transformChild(node->getCondition());

    // Extract condition value
    auto conditionValue = extractLiteralValue(condition.get());

    // If condition is a constant boolean, fold to one branch
    if (conditionValue && std::holds_alternative<bool>(*conditionValue)) {
        bool condBool = std::get<bool>(*conditionValue);

        if (condBool) {
            // Condition is true - return true branch
            if (CF_DEBUG) {
                std::cout << "[CF] ✓ Folded ternary (condition=true) → taking true branch\n";
            }
            auto trueExpr = transformChild(node->getTrueExpression());
            foldedExpressionsCount++;
            modified = true;
            return trueExpr;
        } else {
            // Condition is false - return false branch
            if (CF_DEBUG) {
                std::cout << "[CF] ✓ Folded ternary (condition=false) → taking false branch\n";
            }
            auto falseExpr = transformChild(node->getFalseExpression());
            foldedExpressionsCount++;
            modified = true;
            return falseExpr;
        }
    }

    // Couldn't fold - transform all children and reconstruct
    auto trueExpr = transformChild(node->getTrueExpression());
    auto falseExpr = transformChild(node->getFalseExpression());

    auto newNode = std::make_unique<ast::nodes::expressions::TernaryExpNode>(
        std::move(condition),
        std::move(trueExpr),
        std::move(falseExpr),
        node->getLocation()
    );
    return newNode;
}

// ==================== Type Cast Folding ====================

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::tryFoldCast(
    CastExpression* node
) {
    // Transform expression (bottom-up)
    auto expr = transformChild(node->getExpression());

    // Extract value
    auto exprValue = extractLiteralValue(expr.get());

    // Only fold primitive type casts
    if (exprValue && node->getTargetType()) {
        const auto* targetType = node->getTargetType();

        // Map GenericType to ValueType for primitive types
        ValueType targetValueType;
        bool isPrimitive = false;

        if (targetType->getBaseTypeName() == "int") {
            targetValueType = ValueType::INT;
            isPrimitive = true;
        } else if (targetType->getBaseTypeName() == "float") {
            targetValueType = ValueType::FLOAT;
            isPrimitive = true;
        } else if (targetType->getBaseTypeName() == "bool") {
            targetValueType = ValueType::BOOL;
            isPrimitive = true;
        } else if (targetType->getBaseTypeName() == "String") {
            targetValueType = ValueType::STRING;
            isPrimitive = true;
        }

        if (isPrimitive) {
            auto result = ConstantEvaluator::evaluateTypeCast(*exprValue, targetValueType);

            if (result) {
                auto foldedNode = createLiteralNode(*result, node->getLocation());
                if (foldedNode) {
                    foldedExpressionsCount++;
                    modified = true;

                    if (CF_DEBUG) {
                        std::cout << "[CF] ✓ Folded type cast → " << targetType->getBaseTypeName() << "\n";
                    }

                    return foldedNode;
                }
            }
        }
    }

    // Couldn't fold - reconstruct with transformed expression
    auto newNode = std::make_unique<ast::nodes::expressions::CastExpression>(
        std::move(expr),
        node->getTargetTypeShared(),
        node->getLocation()
    );
    return newNode;
}

// ==================== Expression Visitor Overrides ====================

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitBinaryOpNode(
    BinaryOpNode* node
) {
    return tryFoldBinaryOp(node);
}

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitUnaryOpNode(
    UnaryOpNode* node
) {
    return tryFoldUnaryOp(node);
}

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitTernaryOpNode(
    TernaryOpNode* node
) {
    return tryFoldTernary(node);
}

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitCastExpression(
    CastExpression* node
) {
    return tryFoldCast(node);
}

// ==================== Container Node Visitors (Recursive Traversal) ====================

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitProgramNode(
    ProgramNode* node
) {
    if (CF_DEBUG) {
        std::cout << "[CF] visitProgramNode: Processing " << node->getStatements().size() << " statements\n";
    }

    // Track folded expressions before transforming children
    size_t foldedBefore = foldedExpressionsCount;

    // Transform all statements (this will descend into functions)
    std::vector<std::unique_ptr<ast::ASTNode>> transformedStatements;
    transformedStatements.reserve(node->getStatements().size());

    for (const auto& stmt : node->getStatements()) {
        auto transformed = transformChild(stmt.get());
        transformedStatements.push_back(std::move(transformed));
    }

    // Only create new node if expressions were folded in children
    if (foldedExpressionsCount > foldedBefore) {
        if (CF_DEBUG) {
            std::cout << "[CF] visitProgramNode: Created new ProgramNode with transformed statements\n";
        }
        return std::make_unique<ProgramNode>(std::move(transformedStatements), node->getLocation());
    }

    return nullptr; // No changes
}

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitBlockNode(
    BlockNode* node
) {
    if (CF_DEBUG) {
        std::cout << "[CF] visitBlockNode: Processing " << node->getStatements().size() << " statements\n";
    }

    // Track folded expressions before transforming children
    size_t foldedBefore = foldedExpressionsCount;

    // Transform all statements
    std::vector<std::unique_ptr<ast::ASTNode>> transformedStatements;
    transformedStatements.reserve(node->getStatements().size());

    for (const auto& stmt : node->getStatements()) {
        auto transformed = transformChild(stmt.get());
        transformedStatements.push_back(std::move(transformed));
    }

    // Only create new node if expressions were folded in children
    if (foldedExpressionsCount > foldedBefore) {
        if (CF_DEBUG) {
            std::cout << "[CF] visitBlockNode: Created new BlockNode with transformed statements\n";
        }
        return std::make_unique<BlockNode>(std::move(transformedStatements), node->getLocation());
    }

    return nullptr; // No changes
}

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitFunctionNode(
    FunctionNode* node
) {
    if (CF_DEBUG) {
        std::cout << "[CF] visitFunctionNode: Processing function '" << node->getName() << "'\n";
    }

    // Transform the function body
    auto body = node->getBodyPtr();
    if (!body) {
        return nullptr; // No body
    }

    // Track folded expressions before transforming children
    size_t foldedBefore = foldedExpressionsCount;

    auto transformedBody = transformChild(body);

    // Only create new node if expressions were folded in children
    if (foldedExpressionsCount > foldedBefore) {
        if (CF_DEBUG) {
            std::cout << "[CF] visitFunctionNode: Body was transformed, creating new FunctionNode\n";
        }

        // Convert unique_ptr to shared_ptr for FunctionNode constructor
        std::shared_ptr<ast::ASTNode> transformedBodyShared = std::move(transformedBody);

        auto transformedFunction = std::make_unique<FunctionNode>(
            node->getName(),
            node->getGenericReturnType(),
            node->getGenericParameters(),
            transformedBodyShared,
            node->getGenericTypeParameters(),
            node->getIsAsync(),
            node->getLocation()
        );

        // Preserve visibility modifier
        transformedFunction->setVisibility(node->getVisibility());

        return transformedFunction;
    }

    return nullptr; // No changes
}

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitMethodNode(
    MethodNode* node
) {
    return ASTTransformer::visitMethodNode(node);
}

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitConstructorNode(
    ConstructorNode* node
) {
    return ASTTransformer::visitConstructorNode(node);
}

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitIfNode(
    IfNode* node
) {
    return ASTTransformer::visitIfNode(node);
}

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitWhileNode(
    WhileNode* node
) {
    return ASTTransformer::visitWhileNode(node);
}

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitForNode(
    ForNode* node
) {
    return ASTTransformer::visitForNode(node);
}

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitAssignmentNode(
    AssignmentNode* node
) {
    if (CF_DEBUG) {
        std::cout << "[CF] visitAssignmentNode: Processing assignment to '" << node->getVariableName() << "'\n";
        std::cout << "[CF] visitAssignmentNode: Value node type: " << typeid(*node->getValue()).name() << "\n";
    }

    // Transform the value expression
    auto value = node->getValue();
    if (!value) {
        return nullptr;
    }

    // Track folded expressions before transforming children
    size_t foldedBefore = foldedExpressionsCount;

    auto transformedValue = transformChild(value);

    if (CF_DEBUG) {
        std::cout << "[CF] visitAssignmentNode: After transformChild, result is "
                  << (transformedValue ? "NON-NULL" : "NULL") << "\n";
    }

    // Only create new node if expressions were folded in children
    if (foldedExpressionsCount > foldedBefore) {
        if (CF_DEBUG) {
            std::cout << "[CF] visitAssignmentNode: Value was transformed\n";
        }

        auto newAssignment = std::make_unique<ast::nodes::statements::AssignmentNode>(
            node->getVariableName(),
            std::move(transformedValue),
            node->getVariableType(),
            node->getClassName(),
            node->getIsFinal(),
            node->getIsStatic(),
            node->getLocation()
        );

        newAssignment->setVisibility(node->getVisibility());
        return newAssignment;
    }

    return nullptr;
}

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitReturnNode(
    ReturnNode* node
) {
    return ASTTransformer::visitReturnNode(node);
}

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitClassNode(
    ClassNode* node
) {
    return ASTTransformer::visitClassNode(node);
}

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::CFTransformer::visitArrayLiteralNode(
    ArrayLiteralNode* node
) {
    return ASTTransformer::visitArrayLiteralNode(node);
}

// ==================== ConstantFoldingPass Implementation ====================

ConstantFoldingPass::ConstantFoldingPass()
    : OptimizationPass("ConstantFolding", base::PassType::TRANSFORMATION),
      foldedExpressions(0) {
    // No dependencies for constant folding
    dependencies = {};
}

std::unique_ptr<ast::ASTNode> ConstantFoldingPass::optimize(
    std::unique_ptr<ast::ASTNode> node,
    base::OptimizationContext& context
) {
    if (CF_DEBUG) {
        std::cout << "\n[CF] ===== Starting Constant Folding Pass =====\n";
        std::cout << "[CF] Input node type: " << (node ? typeid(*node).name() : "nullptr") << "\n";
    }

    // Reset metrics
    foldedExpressions = 0;
    transformationCount = 0;

    // Create transformer and run
    auto startTime = std::chrono::high_resolution_clock::now();

    CFTransformer transformer(&context, foldedExpressions);

    // Manual dispatch - check node type and call appropriate visit method
    std::unique_ptr<ast::ASTNode> result;
    if (auto* programNode = dynamic_cast<ProgramNode*>(node.get())) {
        if (CF_DEBUG) {
            std::cout << "[CF] Detected ProgramNode, calling visitProgramNode()\n";
        }
        result = transformer.visitProgramNode(programNode);
    }
    else if (auto* blockNode = dynamic_cast<BlockNode*>(node.get())) {
        if (CF_DEBUG) {
            std::cout << "[CF] Detected BlockNode, calling visitBlockNode()\n";
        }
        result = transformer.visitBlockNode(blockNode);
    }
    else if (auto* functionNode = dynamic_cast<FunctionNode*>(node.get())) {
        if (CF_DEBUG) {
            std::cout << "[CF] Detected FunctionNode, calling visitFunctionNode()\n";
        }
        result = transformer.visitFunctionNode(functionNode);
    }
    else {
        if (CF_DEBUG) {
            std::cout << "[CF] WARNING: Unknown node type, no transformation applied\n";
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // Update transformation count
    transformationCount = foldedExpressions;

    // Mark context as modified if we folded any expressions
    if (transformer.wasModified()) {
        context.setModified(true);
    }

    if (CF_DEBUG) {
        std::cout << "[CF] Transformation result: " << (result ? "NEW NODE" : "nullptr (using original)") << "\n";
        std::cout << "[CF] Folded expressions: " << foldedExpressions << "\n";
        std::cout << "[CF] ===== Constant Folding Pass Complete =====\n\n";
    }

    // If transformation occurred, return transformed node; otherwise return original
    if (result) {
        return result;
    }

    return node;
}

std::string ConstantFoldingPass::getDescription() const {
    return "Constant Folding: Evaluates and folds constant expressions at compile-time";
}

void ConstantFoldingPass::reportMetrics(OptimizationResult& result) const {
    PassMetrics metrics;
    metrics.passName = getName();
    metrics.transformationsApplied = foldedExpressions;
    metrics.executionTime = executionTime;
    metrics.modified = (foldedExpressions > 0);

    result.addPassMetrics(metrics);
}

void ConstantFoldingPass::reset() {
    foldedExpressions = 0;
    transformationCount = 0;
    executionTime = std::chrono::milliseconds(0);
}

} // namespace optimizer::passes
