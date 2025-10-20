#pragma once
#include "base/EvaluationContext.hpp"
#include "managers/InstanceManager.hpp"
#include "utils/GenericTypeManager.hpp"
#include "utils/NodeDispatcher.hpp"
#include "interfaces/IObjectEvaluator.hpp"
#include "../ast/NodeClassesDeclaration.hpp"
#include <memory>
#include <vector>
#include <optional>
#include <unordered_map>
#include <sstream>


namespace evaluator
{
    using namespace base;
    using namespace managers;
    using namespace utils;
    using namespace value;
    using namespace ast::nodes::classes;
    using namespace ast::nodes::statements;
    using namespace runtimeTypes::klass;

    // Forward declarations for specialized handlers
    namespace objects {
        class ArrayAssignmentHandler;
        class ClassRegistrationHandler;
        class StaticMemberHandler;
        class InstanceOperationHandler;
        class GenericInstantiationHandler;
    }

    /**
     * @brief Refactored Object Evaluator following SOLID principles
     * - Single Responsibility: Only handles object-oriented features
     * - Open/Closed: Extensible through composition
     * - Liskov Substitution: Implements IObjectEvaluator interface
     * - Interface Segregation: Uses specialized interfaces
     * - Dependency Inversion: Depends on abstractions
     *
     * Implements IObjectEvaluator to provide object-oriented evaluation functionality.
     * Clients can depend on the interface for loose coupling and testability.
     */
    class ObjectEvaluator : public interfaces::IObjectEvaluator
    {
    public:
        // Configuration constants
        static constexpr size_t MAX_GENERIC_PARAMETERS = 20;

    private:
        std::shared_ptr<EvaluationContext> context;
        std::unique_ptr<InstanceManager> instanceManager;

        // Specialized object handlers
        std::unique_ptr<objects::ArrayAssignmentHandler> arrayAssignmentHandler;
        std::unique_ptr<objects::ClassRegistrationHandler> classRegistrationHandler;
        std::unique_ptr<objects::StaticMemberHandler> staticMemberHandler;
        std::unique_ptr<objects::InstanceOperationHandler> instanceOperationHandler;
        std::unique_ptr<objects::GenericInstantiationHandler> genericInstantiationHandler;

        // Node dispatcher for O(1) dispatch instead of cascading dynamic_cast
        utils::NodeDispatcher<ObjectEvaluator> dispatcher;

        // Forward declarations for circular dependency resolution
        class ExpressionEvaluator* exprEvaluator;
        class StatementEvaluator* stmtEvaluator;

    public:
        explicit ObjectEvaluator(std::shared_ptr<EvaluationContext> ctx);
        ~ObjectEvaluator() override;

        // IObjectEvaluator interface implementation
        Value evaluate(ASTNode* node) override;
        bool canHandle(ASTNode* node) const override;

        // Interface method implementations
        Value callMethod(std::shared_ptr<ObjectInstance> instance, const std::string& methodName,
                         const std::vector<Value>& args) override;
        Value callMethod(std::shared_ptr<ObjectInstance> instance, const std::string& methodName,
                         const std::vector<Value>& args,
                         const errors::SourceLocation& location) override;
        std::shared_ptr<ObjectInstance> createInstance(const std::string& className,
                                                       const std::vector<Value>& constructorArgs) override;
        std::shared_ptr<ObjectInstance> createInstanceWithTypeBindings(const std::string& className,
                                                       const std::vector<Value>& constructorArgs,
                                                       const std::unordered_map<std::string, std::string>& typeBindings) override;

        // Interface method implementations with override
        Value accessStaticMember(const std::string& className, const std::string& memberName) override;
        void assignStaticMember(const std::string& className, const std::string& memberName,
                                const Value& value) override;
        Value callStaticMethod(const std::string& className, const std::string& methodName,
                               const std::vector<Value>& args,
                               const errors::SourceLocation& location = errors::SourceLocation{}) override;
        Value accessMember(std::shared_ptr<ObjectInstance> object, const std::string& memberName,
                          const errors::SourceLocation& location = errors::SourceLocation{}) override;
        void assignMember(std::shared_ptr<ObjectInstance> object, const std::string& memberName,
                          const Value& value,
                          const errors::SourceLocation& location = errors::SourceLocation{}) override;
        Value evaluateMemberAccessNode(MemberAccessNode* node) override;
        Value evaluateMethodCallNode(MethodCallNode* node) override;
        std::string resolveTypeParameterFromContext(const std::string& typeParam) override;

        // Additional public methods (not part of interface)

        // Class and object evaluation methods
        Value evaluateClassNode(ClassNode* node);
        Value evaluateInterfaceNode(InterfaceNode* node);
        Value evaluateMethodNode(MethodNode* node);
        Value evaluateFieldNode(FieldNode* node);
        Value evaluateConstructorNode(ConstructorNode* node);
        Value evaluateNewNode(NewNode* node);
        Value evaluateMemberAssignmentNode(MemberAssignmentNode* node);
        Value evaluateIndexAssignmentNode(IndexAssignmentNode* node);

        // Overload for callStaticMethod with generic type arguments (not in interface)
        Value callStaticMethod(const std::string& className, const std::string& methodName,
                               const std::vector<Value>& args,
                               const std::vector<std::string>& genericTypeArguments,
                               const errors::SourceLocation& location = errors::SourceLocation{});

        // Dependency injection for cross-evaluator communication
        void setExpressionEvaluator(ExpressionEvaluator* evaluator);
        void setStatementEvaluator(StatementEvaluator* evaluator);

    private:
        // Initialize dispatcher with all handler registrations
        void initializeDispatcher();

        // Helper methods
        bool isObjectNode(ASTNode* node) const;
        void registerClass(std::shared_ptr<ClassDefinition> classDef);
        std::vector<Value> evaluateArgumentList(const std::vector<std::unique_ptr<ASTNode>>& args);
    };
}
