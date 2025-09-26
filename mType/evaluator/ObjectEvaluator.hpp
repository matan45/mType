#pragma once
#include "base/EvaluationContext.hpp"
#include "managers/InstanceManager.hpp"
#include "utils/GenericTypeManager.hpp"
#include "../ast/NodeClassesDeclaration.hpp"
#include <memory>
#include <vector>
#include <optional>


namespace evaluator
{
    using namespace base;
    using namespace managers;
    using namespace utils;
    using namespace value;
    using namespace ast::nodes::classes;
    using namespace ast::nodes::statements;
    using namespace runtimeTypes::klass;

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
    private:
        std::shared_ptr<EvaluationContext> context;
        std::unique_ptr<InstanceManager> instanceManager;

        // Forward declarations for circular dependency resolution
        class ExpressionEvaluator* exprEvaluator;
        class StatementEvaluator* stmtEvaluator;

    public:
        explicit ObjectEvaluator(std::shared_ptr<EvaluationContext> ctx);
        ~ObjectEvaluator() = default;

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
        Value accessMember(std::shared_ptr<ObjectInstance> object, const std::string& memberName);
        void assignMember(std::shared_ptr<ObjectInstance> object, const std::string& memberName,
                          const Value& value);
        Value callMethod(std::shared_ptr<ObjectInstance> object, const std::string& methodName,
                         const std::vector<Value>& args,
                         const errors::SourceLocation& location = errors::SourceLocation{});


        // Dependency injection for cross-evaluator communication
        void setExpressionEvaluator(ExpressionEvaluator* evaluator);
        void setStatementEvaluator(StatementEvaluator* evaluator);

        // Generic type resolution
        std::string resolveTypeParameterFromContext(const std::string& typeParam);

    private:
        // Helper methods
        bool isObjectNode(ASTNode* node) const;
        void registerClass(std::shared_ptr<ClassDefinition> classDef);
        std::vector<Value> evaluateArgumentList(const std::vector<std::unique_ptr<ASTNode>>& args);

        // Multi-dimensional array assignment helpers
        std::optional<std::pair<Value, std::vector<size_t>>> extractMultiDimensionalAssignment(IndexAssignmentNode* node);
        Value performDirectMultiDimensionalAssignment(const Value& baseArray, const std::vector<size_t>& indices, const Value& newValue, const SourceLocation& location);
    };
}
