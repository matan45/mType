#pragma once
#include <vector>
#include <string>
#include <utility>
#include "../../vlaue/ValueType.hpp"
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
        ASTNode* body;
        ASTNode* initializerList;  // For member initialization
      public:
       explicit ConstructorDefinition(const std::vector<std::pair<std::string, ValueType>>& params, 
                             ASTNode* b)
            : Definition("constructor"), parameters(params), body(b),
              initializerList(nullptr) {}
        
        bool matchesArgCount(size_t argCount) const;
        
        // Getter methods
        const std::vector<std::pair<std::string, ValueType>>& getParameters() const { return parameters; }
        ASTNode* getBody() const { return body; }
        ASTNode* getInitializerList() const { return initializerList; }
        size_t getParameterCount() const { return parameters.size(); }
        
        // Setter methods
        void setBody(ASTNode* b) { body = b; }
        void setInitializerList(ASTNode* init) { initializerList = init; }
    };
}
