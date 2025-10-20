#pragma once
#include "base/EvaluationContext.hpp"
#include "managers/ControlFlowManager.hpp"
#include "utils/ValueConverter.hpp"
#include "utils/NodeDispatcher.hpp"
#include "interfaces/IStatementEvaluator.hpp"
#include "../ast/NodeClassesDeclaration.hpp"
#include "../ast/nodes/statements/BreakNode.hpp"
#include "../errors/SourceLocation.hpp"
#include <memory>

// Forward declarations for specialized handlers
namespace evaluator {
namespace statements {
    class LoopEvaluator;
    class DeclarationHandler;
    class ControlFlowHandler;
    class ImportAndFunctionHandler;
    class ExceptionHandler;
}
}

namespace evaluator
{
    using namespace base;
    using namespace managers;
    using namespace utils;
    using namespace value;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;

    /**
     * @brief Refactored Statement Evaluator following SOLID principles
     * - Single Responsibility: Only handles statement evaluation
     * - Open/Closed: Extensible through composition
     * - Liskov Substitution: Implements IStatementEvaluator interface
     * - Interface Segregation: Uses specialized interfaces
     * - Dependency Inversion: Depends on abstractions
     *
     * Implements IStatementEvaluator to provide statement evaluation functionality.
     * Clients can depend on the interface for loose coupling and testability.
     */
    class StatementEvaluator : public interfaces::IStatementEvaluator
    {
    private:
        std::shared_ptr<EvaluationContext> context;
        std::unique_ptr<ControlFlowManager> flowManager;

        // Specialized statement handlers
        std::unique_ptr<statements::LoopEvaluator> loopEvaluator;
        std::unique_ptr<statements::DeclarationHandler> declarationHandler;
        std::unique_ptr<statements::ControlFlowHandler> controlFlowHandler;
        std::unique_ptr<statements::ImportAndFunctionHandler> importAndFunctionHandler;
        std::unique_ptr<statements::ExceptionHandler> exceptionHandler;

        // Node dispatcher for O(1) dispatch instead of cascading dynamic_cast
        utils::NodeDispatcher<StatementEvaluator> dispatcher;

        // Forward declarations for circular dependency resolution
        class ExpressionEvaluator* exprEvaluator;
        class ObjectEvaluator* objEvaluator;

        // Helper method for import evaluation
        void evaluateRecursively(ASTNode* node);

    public:
        explicit StatementEvaluator(std::shared_ptr<EvaluationContext> ctx);
        ~StatementEvaluator() override;

        // IStatementEvaluator interface implementation
        Value evaluate(ASTNode* node) override;
        bool canHandle(ASTNode* node) const override;
        Value convertLambdaToInterface(const Value& lambdaValue, const std::string& interfaceName,
                                       const errors::SourceLocation& location = errors::SourceLocation()) override;

        // Control flow management methods (not part of interface)
        bool shouldBreakOrContinue() const;
        void handleBreak();
        void handleContinue();
        void resetLoopFlags();

        // Loop depth management
        void enterLoop();
        void exitLoop();
        bool isInLoop() const;
        void enterBreakableContext(); // Generic method for entering any breakable context
        void exitBreakableContext();  // Generic method for exiting any breakable context
        bool isInBreakableContext() const; // Generic method for checking if break is valid

        // Statement evaluation methods
        Value evaluateProgramNode(ProgramNode* node);
        Value evaluateBlockNode(BlockNode* node);
        Value evaluateAssignmentNode(AssignmentNode* node);
        Value evaluateIfNode(IfNode* node);
        Value evaluateWhileNode(WhileNode* node);
        Value evaluateDoWhileNode(DoWhileNode* node);
        Value evaluateForNode(ForNode* node);
        Value evaluateForEachNode(ForEachNode* node);
        Value evaluateBreakNode(BreakNode* node);
        Value evaluateContinueNode(ContinueNode* node);
        Value evaluateSwitchNode(SwitchNode* node);
        Value evaluateCaseNode(CaseNode* node);
        Value evaluateDefaultCaseNode(DefaultCaseNode* node);
        Value evaluateImportNode(ImportNode* node);
        Value evaluateFunctionNode(FunctionNode* node);
        Value evaluateReturnNode(ReturnNode* node);
        Value evaluateTryNode(TryNode* node);
        Value evaluateCatchNode(CatchNode* node);
        Value evaluateThrowNode(ThrowNode* node);

        // Dependency injection for cross-evaluator communication
        void setExpressionEvaluator(ExpressionEvaluator* evaluator);
        void setObjectEvaluator(ObjectEvaluator* evaluator);

    private:
        // Initialize dispatcher with all handler registrations
        void initializeDispatcher();

        // Helper methods
        bool isStatementNode(ASTNode* node) const;
        Value executeStatementList(const std::vector<std::unique_ptr<ASTNode>>& statements);
        void validateAssignmentAsDeclaration(AssignmentNode* node);
        void validateTypeAssignment(ValueType expectedType, const Value& value,
                                    const std::string& variableName,
                                    const SourceLocation& location);
        void validateTypeAssignment(ValueType expectedType, const Value& value,
                                    const std::string& variableName,
                                    const SourceLocation& location,
                                    const std::string& expectedClassName);
        void validateClassExists(const std::string& className, const SourceLocation& location);
        bool isValidTypeConversion(ValueType from, ValueType to);
        bool isGenericTypeCompatible(const std::string& actualClassName,
                                   const std::string& expectedClassName);
        void validateObjectTypeCompatibility(const Value& value, const std::string& variableName,
                                             const SourceLocation& location,
                                             const std::string& expectedClassName);
    };
}
