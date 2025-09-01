#pragma once
#include <vector>
#include <utility>
#include <memory>
#include "../../value/ValueType.hpp"
#include "../../ast/ASTNode.hpp"
#include "../Definition.hpp"


namespace runtimeTypes::klass
{
    using namespace value;
    using namespace ast;

    class MethodDefinition : public Definition
    {
    private:
        ValueType returnType;
        std::vector<std::pair<std::string, ValueType>> parameters;
        std::vector<std::pair<std::string, Value>> arguments;
        std::shared_ptr<ASTNode> body;
        bool isStaticMethod;

    public:
        explicit MethodDefinition(const std::string& n, ValueType rt,
                         const std::vector<std::pair<std::string, ValueType>>& params,
                         const std::vector<std::pair<std::string, Value>>&args,
                         std::shared_ptr<ASTNode> b, bool s)
            : Definition(n), returnType(rt), parameters(params), arguments(args), body(b), isStaticMethod(s)
             
        {
        }

        bool matchesArgCount(size_t argCount) const;
        
        const ValueType& getReturnType() const { return returnType; }
        void setReturnType(const ValueType& rt) { returnType = rt; }
        
        const std::vector<std::pair<std::string, ValueType>>& getParameters() const { return parameters; }
        void setParameters(const std::vector<std::pair<std::string, ValueType>>& params) { parameters = params; }
        
        ASTNode* getBody() const { return body.get(); }
        void setBody(std::shared_ptr<ASTNode> b) { body = b; }
        
        bool isStatic() const { return isStaticMethod; }
        void setStatic(bool s) { isStaticMethod = s; }
    };
}
