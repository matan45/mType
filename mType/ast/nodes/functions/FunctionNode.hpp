#pragma once
#include "../../ASTNode.hpp"
#include "../../../value/ValueType.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ast::nodes::functions
{
    using namespace value;

    class FunctionNode : public ASTNode
    {
    private:
        std::string name;
        ValueType returnType;
        std::vector<std::pair<std::string, ValueType>> parameters;
        std::shared_ptr<ASTNode> body;

    public:
        // Constructor accepting shared_ptr
        explicit FunctionNode(const std::string& funcName, ValueType retType,
                     std::vector<std::pair<std::string, ValueType>> params,
                     std::shared_ptr<ASTNode> funcBody,
                     const SourceLocation& loc = SourceLocation());
        
        // Constructor accepting unique_ptr for backward compatibility
        explicit FunctionNode(const std::string& funcName, ValueType retType,
                     std::vector<std::pair<std::string, ValueType>> params,
                     std::unique_ptr<ASTNode> funcBody,
                     const SourceLocation& loc = SourceLocation());

        const std::string& getName() const;
        ValueType getReturnType() const;
        const std::vector<std::pair<std::string, ValueType>>& getParameters() const;
        
        // Safe getter - returns shared_ptr
        [[nodiscard]] std::shared_ptr<ASTNode> getBody() const;
        
        // For code that just needs to read
        [[nodiscard]] ASTNode* getBodyPtr() const noexcept;

        void setName(const std::string& funcName);
        void setReturnType(ValueType retType);
        void setParameters(std::vector<std::pair<std::string, ValueType>> params);
        void setBody(std::shared_ptr<ASTNode> funcBody);

        size_t getParameterCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
       
    };
}
