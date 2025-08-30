#pragma once
#include <vector>
#include <string>
#include <utility>
#include <memory>
#include "../../value/ValueType.hpp"
#include "../../ast/ASTNode.hpp"
#include "../Definition.hpp"

namespace runtimeTypes::klass
{
    using namespace value;
    using namespace ast;
    
    class ConstructorDefinition : public Definition
    {
    private:
        std::vector<std::pair<std::string, ValueType>> parameters;
        std::shared_ptr<ASTNode> body;
        std::shared_ptr<ASTNode> initializerList;  // For member initialization
      public:
       explicit ConstructorDefinition(const std::vector<std::pair<std::string, ValueType>>& params, 
                             std::shared_ptr<ASTNode> b)
            : Definition("constructor"), parameters(params), body(b),
              initializerList(nullptr) {}
        
        bool matchesArgCount(size_t argCount) const;
        
        // Getter methods
        const std::vector<std::pair<std::string, ValueType>>& getParameters() const { return parameters; }
        ASTNode* getBody() const { return body.get(); }
        ASTNode* getInitializerList() const { return initializerList.get(); }
        size_t getParameterCount() const { return parameters.size(); }
        
        // Setter methods
        void setBody(std::shared_ptr<ASTNode> b) { body = b; }
        void setInitializerList(std::shared_ptr<ASTNode> init) { initializerList = init; }
    };
}
