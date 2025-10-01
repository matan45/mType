#pragma once
#include "base/EvaluationContext.hpp"
#include "managers/InstanceManager.hpp"
#include "utils/GenericTypeManager.hpp"
#include "utils/NodeDispatcher.hpp"
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
     */
    class ObjectEvaluator
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
        ~ObjectEvaluator();

        // IEvaluator interface implementation
        Value evaluate(ASTNode* node);
        bool canHandle(ASTNode* node) const;

        // IObjectEvaluator interface implementation

        // Class and object evaluation methods
        Value evaluateClassNode(ClassNode* node);
        Value evaluateInterfaceNode(InterfaceNode* node);
        Value evaluateMethodNode(MethodNode* node);
        Value evaluateFieldNode(FieldNode* node);
        Value evaluateConstructorNode(ConstructorNode* node);
        Value evaluateNewNode(NewNode* node);
        Value evaluateMemberAccessNode(MemberAccessNode* node);
        Value evaluateMethodCallNode(MethodCallNode* node);
        Value evaluateMemberAssignmentNode(MemberAssignmentNode* node);
        Value evaluateIndexAssignmentNode(IndexAssignmentNode* node);

        // Static member operations
        Value accessStaticMember(const std::string& className, const std::string& memberName);
        void assignStaticMember(const std::string& className, const std::string& memberName,
                                const Value& value);
        Value callStaticMethod(const std::string& className, const std::string& methodName,
                               const std::vector<Value>& args,
                               const errors::SourceLocation& location = errors::SourceLocation{});

        Value callStaticMethod(const std::string& className, const std::string& methodName,
                               const std::vector<Value>& args,
                               const std::vector<std::string>& genericTypeArguments,
                               const errors::SourceLocation& location = errors::SourceLocation{});

        // Instance operations
        std::shared_ptr<ObjectInstance> createInstance(const std::string& className,
                                                       const std::vector<Value>& constructorArgs);
        std::shared_ptr<ObjectInstance> createInstanceWithTypeBindings(const std::string& className,
                                                       const std::vector<Value>& constructorArgs,
                                                       const std::unordered_map<std::string, std::string>& typeBindings);
        Value accessMember(std::shared_ptr<ObjectInstance> object, const std::string& memberName,
                          const errors::SourceLocation& location = errors::SourceLocation{});
        void assignMember(std::shared_ptr<ObjectInstance> object, const std::string& memberName,
                          const Value& value,
                          const errors::SourceLocation& location = errors::SourceLocation{});
        Value callMethod(std::shared_ptr<ObjectInstance> object, const std::string& methodName,
                         const std::vector<Value>& args,
                         const errors::SourceLocation& location = errors::SourceLocation{});


        // Dependency injection for cross-evaluator communication
        void setExpressionEvaluator(ExpressionEvaluator* evaluator);
        void setStatementEvaluator(StatementEvaluator* evaluator);

        // Generic type resolution
        std::string resolveTypeParameterFromContext(const std::string& typeParam);

    private:
        // Initialize dispatcher with all handler registrations
        void initializeDispatcher();

        // Helper methods
        bool isObjectNode(ASTNode* node) const;
        void registerClass(std::shared_ptr<ClassDefinition> classDef);
        std::vector<Value> evaluateArgumentList(const std::vector<std::unique_ptr<ASTNode>>& args);
        void validateInterfaceImplementations(std::shared_ptr<ClassDefinition> classDef, ClassNode* node);
        std::pair<std::string, std::vector<std::string>> parseGenericInterfaceName(const std::string& interfaceName);
        std::string resolveGenericType(const std::string& typeName, const std::unordered_map<std::string, std::string>& typeSubstitutions);
    };
}
