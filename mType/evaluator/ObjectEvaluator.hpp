#pragma once
#include "../value/ValueType.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../ast/nodes/classes/FieldNode.hpp"
#include "../ast/nodes/classes/ConstructorNode.hpp"
#include "../ast/nodes/classes/NewNode.hpp"
#include "../ast/nodes/classes/MemberAccessNode.hpp"
#include "../ast/nodes/classes/MethodCallNode.hpp"
#include "../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include <memory>
#include <vector>

namespace evaluator
{
    using namespace value;
    using namespace ast::nodes::classes;
    using namespace ast::nodes::statements;
    using namespace runtimeTypes::klass;
    
    class Evaluator;
    
    class ObjectEvaluator
    {
    private:
        Evaluator* mainEvaluator;
        std::shared_ptr<ObjectInstance> currentInstance;
        
    public:
        explicit ObjectEvaluator(Evaluator* evaluator);
        ~ObjectEvaluator() = default;
        
        // Class and object evaluation methods
        Value evaluateClassNode(ClassNode* node);
        Value evaluateMethodNode(MethodNode* node);
        Value evaluateFieldNode(FieldNode* node);
        Value evaluateConstructorNode(ConstructorNode* node);
        Value evaluateNewNode(NewNode* node);
        Value evaluateMemberAccessNode(MemberAccessNode* node);
        Value evaluateMethodCallNode(MethodCallNode* node);
        Value evaluateMemberAssignmentNode(MemberAssignmentNode* node);
        
        // Helper methods
        std::shared_ptr<ObjectInstance> createInstance(const std::string& className,
                                                      const std::vector<Value>& constructorArgs);
        Value accessMember(std::shared_ptr<ObjectInstance> object, const std::string& memberName);
        void assignMember(std::shared_ptr<ObjectInstance> object, const std::string& memberName, 
                         const Value& value);
        Value callMethod(std::shared_ptr<ObjectInstance> object, const std::string& methodName,
                        const std::vector<Value>& args);
        
        // Current instance management (for 'this' keyword)
        void setCurrentInstance(std::shared_ptr<ObjectInstance> instance);
        std::shared_ptr<ObjectInstance> getCurrentInstance() const;
        void clearCurrentInstance();
        
        // Static member handling
        Value accessStaticMember(const std::string& className, const std::string& memberName);
        void assignStaticMember(const std::string& className, const std::string& memberName,
                               const Value& value);
        Value callStaticMethod(const std::string& className, const std::string& methodName,
                              const std::vector<Value>& args);
    };
}
