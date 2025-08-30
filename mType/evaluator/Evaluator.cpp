#include "Evaluator.hpp"
#include "ExpressionEvaluator.hpp"
#include "StatementEvaluator.hpp"
#include "ObjectEvaluator.hpp"
#include "NamespaceEvaluator.hpp"
#include "../exception/ReturnException.hpp"
#include "../exception/BreakException.hpp"
#include "../exception/ContinueException.hpp"
#include <iostream>

namespace evaluator
{
    Evaluator::Evaluator(std::shared_ptr<Environment> environment)
        : env(environment), hasReturned(false)
    {
        exprEvaluator = std::make_unique<ExpressionEvaluator>(this);
        stmtEvaluator = std::make_unique<StatementEvaluator>(this);
        objEvaluator = std::make_unique<ObjectEvaluator>(this);
        nsEvaluator = std::make_unique<NamespaceEvaluator>(this);
    }

    Evaluator::~Evaluator() = default;

    Value Evaluator::evaluate(ASTNode* node)
    {
        if (!node) {
            return std::monostate{};
        }
        return node->accept(*this);
    }

    Value Evaluator::getReturnValue()
    {
        if (!returnStack.empty()) {
            Value val = returnStack.top();
            returnStack.pop();
            return val;
        }
        return std::monostate{};
    }

    void Evaluator::pushReturnValue(const Value& value)
    {
        returnStack.push(value);
    }

    // Program and block nodes
    Value Evaluator::visitProgramNode(ProgramNode* node)
    {
        return stmtEvaluator->evaluateProgramNode(node);
    }

    Value Evaluator::visitBlockNode(BlockNode* node)
    {
        return stmtEvaluator->evaluateBlockNode(node);
    }

    // Literal nodes
    Value Evaluator::visitFloatNode(FloatNode* node)
    {
        return exprEvaluator->evaluateFloatNode(node);
    }

    Value Evaluator::visitIntegerNode(IntegerNode* node)
    {
        return exprEvaluator->evaluateIntegerNode(node);
    }

    Value Evaluator::visitStringNode(StringNode* node)
    {
        return exprEvaluator->evaluateStringNode(node);
    }

    Value Evaluator::visitBoolNode(BoolNode* node)
    {
        return exprEvaluator->evaluateBoolNode(node);
    }

    Value Evaluator::visitNullNode(NullNode* node)
    {
        return exprEvaluator->evaluateNullNode(node);
    }

    // Variable and assignment nodes
    Value Evaluator::visitVariableNode(VariableNode* node)
    {
        return exprEvaluator->evaluateVariableNode(node);
    }

    Value Evaluator::visitDeclarationNode(DeclarationNode* node)
    {
        return stmtEvaluator->evaluateDeclarationNode(node);
    }

    Value Evaluator::visitAssignmentNode(AssignmentNode* node)
    {
        return stmtEvaluator->evaluateAssignmentNode(node);
    }

    Value Evaluator::visitQualifiedAssignmentNode(QualifiedAssignmentNode* node)
    {
        return nsEvaluator->evaluateQualifiedAssignmentNode(node);
    }

    Value Evaluator::visitMemberAssignmentNode(MemberAssignmentNode* node)
    {
        return objEvaluator->evaluateMemberAssignmentNode(node);
    }

    // Expression nodes
    Value Evaluator::visitBinaryOpNode(BinaryOpNode* node)
    {
        return exprEvaluator->evaluateBinaryExpNode(node);
    }

    Value Evaluator::visitTernaryOpNode(TernaryOpNode* node)
    {
        return exprEvaluator->evaluateTernaryExpNode(node);
    }

    Value Evaluator::visitUnaryOpNode(UnaryOpNode* node)
    {
        return exprEvaluator->evaluateUnaryExpNode(node);
    }

    // Control flow nodes
    Value Evaluator::visitIfNode(IfNode* node)
    {
        return stmtEvaluator->evaluateIfNode(node);
    }

    Value Evaluator::visitWhileNode(WhileNode* node)
    {
        return stmtEvaluator->evaluateWhileNode(node);
    }

    Value Evaluator::visitDoWhileNode(DoWhileNode* node)
    {
        return stmtEvaluator->evaluateDoWhileNode(node);
    }

    Value Evaluator::visitForNode(ForNode* node)
    {
        return stmtEvaluator->evaluateForNode(node);
    }

    Value Evaluator::visitBreakNode(BreakNode* node)
    {
        return stmtEvaluator->evaluateBreakNode(node);
    }

    Value Evaluator::visitContinueNode(ContinueNode* node)
    {
        return stmtEvaluator->evaluateContinueNode(node);
    }

    Value Evaluator::visitSwitchNode(SwitchNode* node)
    {
        return stmtEvaluator->evaluateSwitchNode(node);
    }

    Value Evaluator::visitCaseNode(CaseNode* node)
    {
        return stmtEvaluator->evaluateCaseNode(node);
    }

    Value Evaluator::visitDefaultCaseNode(DefaultCaseNode* node)
    {
        return stmtEvaluator->evaluateDefaultCaseNode(node);
    }

    // Function nodes
    Value Evaluator::visitFunctionNode(FunctionNode* node)
    {
        return stmtEvaluator->evaluateFunctionNode(node);
    }

    Value Evaluator::visitFunctionCallNode(FunctionCallNode* node)
    {
        return exprEvaluator->evaluateFunctionCallNode(node);
    }

    Value Evaluator::visitReturnNode(ReturnNode* node)
    {
        return stmtEvaluator->evaluateReturnNode(node);
    }

    Value Evaluator::visitNativeFunctionNode(NativeFunctionNode* node)
    {
        return stmtEvaluator->evaluateNativeFunctionNode(node);
    }

    // Import nodes
    Value Evaluator::visitImportNode(ImportNode* node)
    {
        return stmtEvaluator->evaluateImportNode(node);
    }

    // Class nodes
    Value Evaluator::visitClassNode(ClassNode* node)
    {
        return objEvaluator->evaluateClassNode(node);
    }

    Value Evaluator::visitMethodNode(MethodNode* node)
    {
        return objEvaluator->evaluateMethodNode(node);
    }

    Value Evaluator::visitFieldNode(FieldNode* node)
    {
        return objEvaluator->evaluateFieldNode(node);
    }

    Value Evaluator::visitConstructorNode(ConstructorNode* node)
    {
        return objEvaluator->evaluateConstructorNode(node);
    }

    Value Evaluator::visitNewNode(NewNode* node)
    {
        return objEvaluator->evaluateNewNode(node);
    }

    Value Evaluator::visitMemberAccessNode(MemberAccessNode* node)
    {
        return objEvaluator->evaluateMemberAccessNode(node);
    }

    Value Evaluator::visitMethodCallNode(MethodCallNode* node)
    {
        return objEvaluator->evaluateMethodCallNode(node);
    }

    // Namespace nodes
    Value Evaluator::visitNamespaceNode(NamespaceNode* node)
    {
        return nsEvaluator->evaluateNamespaceNode(node);
    }

    Value Evaluator::visitUsingNode(UsingNode* node)
    {
        return nsEvaluator->evaluateUsingNode(node);
    }

    Value Evaluator::visitQualifiedNameNode(QualifiedNameNode* node)
    {
        return nsEvaluator->evaluateQualifiedNameNode(node);
    }

    // Helper method implementations
    bool Evaluator::isTruthy(const Value& value)
    {
        return exprEvaluator->isTruthy(value);
    }

    std::string Evaluator::toString(const Value& value)
    {
        return exprEvaluator->toString(value);
    }

    float Evaluator::toFloat(const Value& value)
    {
        return exprEvaluator->toFloat(value);
    }

    int Evaluator::toInt(const Value& value)
    {
        return exprEvaluator->toInt(value);
    }

    // Cross-evaluator delegation helpers
    Value Evaluator::evaluateObjectMethodCall(MethodCallNode* node)
    {
        return objEvaluator->evaluateMethodCallNode(node);
    }

    Value Evaluator::evaluateObjectCreation(NewNode* node)
    {
        return objEvaluator->evaluateNewNode(node);
    }

    Value Evaluator::evaluateObjectMemberAccess(MemberAccessNode* node)
    {
        return objEvaluator->evaluateMemberAccessNode(node);
    }

    Value Evaluator::evaluateObjectMemberAssignment(MemberAssignmentNode* node)
    {
        return objEvaluator->evaluateMemberAssignmentNode(node);
    }

    Value Evaluator::evaluateQualifiedNameAccess(QualifiedNameNode* node)
    {
        return nsEvaluator->evaluateQualifiedNameNode(node);
    }

    Value Evaluator::evaluateQualifiedAssignment(QualifiedAssignmentNode* node)
    {
        return nsEvaluator->evaluateQualifiedAssignmentNode(node);
    }
    
    std::shared_ptr<runtimeTypes::klass::ObjectInstance> Evaluator::getCurrentInstance() const
    {
        return objEvaluator->getCurrentInstance();
    }
    
    Value Evaluator::callMethodOnInstance(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance, 
                                          const std::string& methodName, const std::vector<Value>& args)
    {
        return objEvaluator->callMethod(instance, methodName, args);
    }
    
    StatementEvaluator* Evaluator::getStatementEvaluator() const
    {
        return stmtEvaluator.get();
    }
}