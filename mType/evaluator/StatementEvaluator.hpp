#pragma once
#include "../value/ValueType.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../ast/nodes/statements/DeclarationNode.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/statements/QualifiedAssignmentNode.hpp"
#include "../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../ast/nodes/statements/IfNode.hpp"
#include "../ast/nodes/statements/WhileNode.hpp"
#include "../ast/nodes/statements/DoWhileNode.hpp"
#include "../ast/nodes/statements/ForNode.hpp"
#include "../ast/nodes/statements/BreakNode.hpp"
#include "../ast/nodes/statements/ContinueNode.hpp"
#include "../ast/nodes/statements/SwitchNode.hpp"
#include "../ast/nodes/statements/CaseNode.hpp"
#include "../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../ast/nodes/statements/ImportNode.hpp"
#include "../ast/nodes/statements/NativeFunctionNode.hpp"
#include "../ast/nodes/functions/FunctionNode.hpp"
#include "../ast/nodes/functions/ReturnNode.hpp"
#include <memory>

namespace evaluator
{
    using namespace value;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;
    
    class Evaluator;
    
    class StatementEvaluator
    {
    private:
        Evaluator* mainEvaluator;
        bool breakFlag;
        bool continueFlag;
        
    public:
        explicit StatementEvaluator(Evaluator* evaluator);
        ~StatementEvaluator() = default;
        
        // Statement evaluation methods
        Value evaluateProgramNode(ProgramNode* node);
        Value evaluateBlockNode(BlockNode* node);
        Value evaluateDeclarationNode(DeclarationNode* node);
        Value evaluateAssignmentNode(AssignmentNode* node);
        Value evaluateQualifiedAssignmentNode(QualifiedAssignmentNode* node);
        Value evaluateMemberAssignmentNode(MemberAssignmentNode* node);
        Value evaluateIfNode(IfNode* node);
        Value evaluateWhileNode(WhileNode* node);
        Value evaluateDoWhileNode(DoWhileNode* node);
        Value evaluateForNode(ForNode* node);
        Value evaluateBreakNode(BreakNode* node);
        Value evaluateContinueNode(ContinueNode* node);
        Value evaluateSwitchNode(SwitchNode* node);
        Value evaluateCaseNode(CaseNode* node);
        Value evaluateDefaultCaseNode(DefaultCaseNode* node);
        Value evaluateImportNode(ImportNode* node);
        Value evaluateFunctionNode(FunctionNode* node);
        Value evaluateReturnNode(ReturnNode* node);
        Value evaluateNativeFunctionNode(NativeFunctionNode* node);
        
        // Helper methods
        bool shouldBreakOrContinue() const;
        void handleBreak();
        void handleContinue();
        bool isBreaking() const;
        bool isContinuing() const;
        void resetLoopFlags();
        
        // Helper for value comparison
        bool compareValues(const Value& left, const Value& right);
    };
}
