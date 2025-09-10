#pragma once
#include "base/EvaluationContext.hpp"
#include "managers/InstanceManager.hpp"
#include "utils/CollectionMethodDispatcher.hpp"
#include "../ast/NodeClassesDeclaration.hpp"
#include <memory>
#include <vector>

// Forward declarations for collection types
namespace runtimeTypes::collections {
    class Array;
    class Map;
    class Set;
    class Stack;
    class Queue;
}

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
        Value evaluateMethodNode(MethodNode* node);
        Value evaluateFieldNode(FieldNode* node);
        Value evaluateConstructorNode(ConstructorNode* node);
        Value evaluateNewNode(NewNode* node);
        Value evaluateMemberAccessNode(MemberAccessNode* node);
        Value evaluateMethodCallNode(MethodCallNode* node);
        Value evaluateMemberAssignmentNode(MemberAssignmentNode* node);

        // Static member operations
        Value accessStaticMember(const std::string& className, const std::string& memberName);
        void assignStaticMember(const std::string& className, const std::string& memberName,
                                const Value& value);
        Value callStaticMethod(const std::string& className, const std::string& methodName,
                               const std::vector<Value>& args);

        // Instance operations
        std::shared_ptr<ObjectInstance> createInstance(const std::string& className,
                                                       const std::vector<Value>& constructorArgs);
        Value accessMember(std::shared_ptr<ObjectInstance> object, const std::string& memberName);
        void assignMember(std::shared_ptr<ObjectInstance> object, const std::string& memberName,
                          const Value& value);
        Value callMethod(std::shared_ptr<ObjectInstance> object, const std::string& methodName,
                         const std::vector<Value>& args);

        // Collection method operations - UNIFIED APPROACH
        Value dispatchCollectionMethod(const Value& collectionValue,
                                     const std::string& methodName, 
                                     const std::vector<Value>& args);
        
        template<typename CollectionType>
        Value callCollectionMethod(std::shared_ptr<CollectionType> collection, 
                                   const std::string& methodName, const std::vector<Value>& args);
        
        // Specialized collection method operations
        Value callArrayMethod(std::shared_ptr<runtimeTypes::collections::Array> array, 
                             const std::string& methodName, const std::vector<Value>& args);
        Value callMapMethod(std::shared_ptr<runtimeTypes::collections::Map> map, 
                           const std::string& methodName, const std::vector<Value>& args);
        Value callSetMethod(std::shared_ptr<runtimeTypes::collections::Set> set, 
                           const std::string& methodName, const std::vector<Value>& args);
        Value callStackMethod(std::shared_ptr<runtimeTypes::collections::Stack> stack, 
                             const std::string& methodName, const std::vector<Value>& args);
        Value callQueueMethod(std::shared_ptr<runtimeTypes::collections::Queue> queue, 
                             const std::string& methodName, const std::vector<Value>& args);

        // Dependency injection for cross-evaluator communication
        void setExpressionEvaluator(ExpressionEvaluator* evaluator);
        void setStatementEvaluator(StatementEvaluator* evaluator);

    private:
        // Helper methods
        bool isObjectNode(ASTNode* node) const;
        void registerClass(std::shared_ptr<ClassDefinition> classDef);
        std::vector<Value> evaluateArgumentList(const std::vector<std::unique_ptr<ASTNode>>& args);
    };

    // Template implementation - Must be in header for templates
    template<typename CollectionType>
    Value ObjectEvaluator::callCollectionMethod(std::shared_ptr<CollectionType> collection,
                                               const std::string& methodName,
                                               const std::vector<Value>& args)
    {
        return utils::CollectionMethodDispatcher<CollectionType>::dispatch(collection, methodName, args);
    }
}
