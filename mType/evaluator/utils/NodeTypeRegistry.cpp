#include "NodeTypeRegistry.hpp"
#include "../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../ast/nodes/expressions/FloatNode.hpp"
#include "../../ast/nodes/expressions/StringNode.hpp"
#include "../../ast/nodes/expressions/BoolNode.hpp"
#include "../../ast/nodes/expressions/VariableNode.hpp"
#include "../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../ast/nodes/expressions/NullNode.hpp"
#include "../../ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../ast/nodes/expressions/LambdaNode.hpp"
#include "../../ast/nodes/expressions/LambdaInterfaceInvocationNode.hpp"
#include "../../ast/nodes/expressions/CastExpression.hpp"
#include "../../ast/nodes/expressions/InstanceOfExpression.hpp"
#include "../../ast/nodes/expressions/AwaitExpression.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/statements/BlockNode.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../ast/nodes/statements/IfNode.hpp"
#include "../../ast/nodes/statements/WhileNode.hpp"
#include "../../ast/nodes/statements/DoWhileNode.hpp"
#include "../../ast/nodes/statements/ForNode.hpp"
#include "../../ast/nodes/statements/ForEachNode.hpp"
#include "../../ast/nodes/statements/BreakNode.hpp"
#include "../../ast/nodes/statements/ContinueNode.hpp"
#include "../../ast/nodes/statements/SwitchNode.hpp"
#include "../../ast/nodes/statements/CaseNode.hpp"
#include "../../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../../ast/nodes/statements/ImportNode.hpp"
#include "../../ast/nodes/functions/ReturnNode.hpp"
#include "../../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../../ast/nodes/statements/IndexAssignmentNode.hpp"
#include "../../ast/nodes/statements/TryNode.hpp"
#include "../../ast/nodes/statements/CatchNode.hpp"
#include "../../ast/nodes/statements/ThrowNode.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../ast/nodes/classes/InterfaceNode.hpp"
#include "../../ast/nodes/classes/MethodNode.hpp"
#include "../../ast/nodes/classes/FieldNode.hpp"
#include "../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../ast/nodes/classes/NewNode.hpp"
#include "../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../ast/nodes/classes/SuperConstructorCallNode.hpp"
#include "../../ast/nodes/classes/SuperMethodCallNode.hpp"

namespace evaluator::utils
{
    // Initialize static registries
    const std::unordered_set<std::type_index> NodeTypeRegistry::expressionTypes = initExpressionTypes();
    const std::unordered_set<std::type_index> NodeTypeRegistry::statementTypes = initStatementTypes();
    const std::unordered_set<std::type_index> NodeTypeRegistry::objectTypes = initObjectTypes();

    std::unordered_set<std::type_index> NodeTypeRegistry::initExpressionTypes()
    {
        return {
            typeid(IntegerNode),
            typeid(FloatNode),
            typeid(StringNode),
            typeid(BoolNode),
            typeid(NullNode),
            typeid(VariableNode),
            typeid(BinaryExpNode),
            typeid(TernaryExpNode),
            typeid(UnaryExpNode),
            typeid(FunctionCallNode),
            typeid(ArrayCreationNode),
            typeid(ArrayLiteralNode),
            typeid(IndexAccessNode),
            typeid(LambdaNode),
            typeid(LambdaInterfaceInvocationNode),
            typeid(CastExpression),         // Type cast expression (Type)expr
            typeid(InstanceOfExpression),   // Type check expression expr isClassOf Type
            typeid(AwaitExpression),        // Await expression for async/await
            typeid(AssignmentNode),         // Assignment can be expression or statement
            typeid(MemberAccessNode),       // Member access is an expression (obj.field, arr.length)
            typeid(MethodCallNode),         // Method calls are expressions (obj.method())
            typeid(NewNode),                // Object creation is an expression (new ClassName())
            typeid(MemberAssignmentNode),   // Member assignment is an expression for chained assignments (obj.field = value)
            typeid(SuperConstructorCallNode), // Super constructor call is an expression (super(...))
            typeid(SuperMethodCallNode)     // Super method call is an expression (super.method())
        };
    }

    std::unordered_set<std::type_index> NodeTypeRegistry::initStatementTypes()
    {
        return {
            typeid(ProgramNode),
            typeid(BlockNode),
            typeid(AssignmentNode),
            typeid(IfNode),
            typeid(WhileNode),
            typeid(DoWhileNode),
            typeid(ForNode),
            typeid(ForEachNode),
            typeid(BreakNode),
            typeid(ContinueNode),
            typeid(SwitchNode),
            typeid(CaseNode),
            typeid(DefaultCaseNode),
            typeid(ImportNode),
            typeid(ReturnNode),
            typeid(FunctionNode),
            typeid(TryNode),
            typeid(CatchNode),
            typeid(ThrowNode)
        };
    }

    std::unordered_set<std::type_index> NodeTypeRegistry::initObjectTypes()
    {
        return {
            typeid(ClassNode),
            typeid(InterfaceNode),
            typeid(MethodNode),
            typeid(FieldNode),
            typeid(ConstructorNode),
            typeid(NewNode),
            typeid(MemberAccessNode),
            typeid(MethodCallNode),
            typeid(MemberAssignmentNode),
            typeid(IndexAssignmentNode)
        };
    }

    NodeTypeRegistry::NodeCategory NodeTypeRegistry::categorize(ASTNode* node)
    {
        if (!node)
        {
            return NodeCategory::UNKNOWN;
        }

        std::type_index nodeType = typeid(*node);

        // Check in priority order: Object > Statement > Expression
        if (objectTypes.find(nodeType) != objectTypes.end())
        {
            return NodeCategory::OBJECT;
        }
        if (statementTypes.find(nodeType) != statementTypes.end())
        {
            return NodeCategory::STATEMENT;
        }
        if (expressionTypes.find(nodeType) != expressionTypes.end())
        {
            return NodeCategory::EXPRESSION;
        }

        return NodeCategory::UNKNOWN;
    }

    bool NodeTypeRegistry::isExpression(ASTNode* node)
    {
        if (!node)
        {
            return false;
        }
        return expressionTypes.find(typeid(*node)) != expressionTypes.end();
    }

    bool NodeTypeRegistry::isStatement(ASTNode* node)
    {
        if (!node)
        {
            return false;
        }
        return statementTypes.find(typeid(*node)) != statementTypes.end();
    }

    bool NodeTypeRegistry::isObject(ASTNode* node)
    {
        if (!node)
        {
            return false;
        }
        return objectTypes.find(typeid(*node)) != objectTypes.end();
    }
}
