#pragma once
#include <vector>
#include "../../value/ValueType.hpp"
#include "../../ast/ASTNode.hpp"
#include "../Definition.hpp"

namespace runtimeTypes::global
{
    using namespace value;
    using namespace ast;
    
    class FunctionDefinition : public Definition
    {
    private:
        ValueType returnType;
        std::vector<std::pair<std::string, ValueType>> parameters;
        ASTNode* body;
        
    public:
        explicit FunctionDefinition(const std::string& name) : Definition(name), returnType(ValueType::VOID), body(nullptr) {}
        
        explicit FunctionDefinition(const std::string& name, ValueType retType, const std::vector<std::pair<std::string, ValueType>>& params)
            : Definition(name), returnType(retType), parameters(params), body(nullptr) {}

        ValueType getReturnType() const { return returnType; }
        void setReturnType(ValueType type) { returnType = type; }
        
        const std::vector<std::pair<std::string, ValueType>>& getParameters() const { return parameters; }
        void setParameters(const std::vector<std::pair<std::string, ValueType>>& params) { parameters = params; }
        
        size_t getParameterCount() const { return parameters.size(); }
        
        ASTNode* getBody() const { return body; }
        void setBody(ASTNode* bodyNode) { body = bodyNode; }
    };
}
