#pragma once
#include "../../ASTNode.hpp"
#include "../../../value/ValueType.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ast::nodes::classes
{
    using namespace value;

    class MethodNode : public ASTNode
    {
    private:
        std::string name;
        ValueType returnType;
        std::vector<std::pair<std::string, ValueType>> parameters;
        std::shared_ptr<ASTNode> body;
        bool isStatic;

    public:
        // Constructor accepting shared_ptr
        explicit MethodNode(const std::string& methodName, ValueType retType,
                   std::vector<std::pair<std::string, ValueType>> params,
                   std::shared_ptr<ASTNode> methodBody, bool isStaticMethod = false,
                   const SourceLocation& loc = SourceLocation());
        
        // Constructor accepting unique_ptr for backward compatibility
        explicit MethodNode(const std::string& methodName, ValueType retType,
                   std::vector<std::pair<std::string, ValueType>> params,
                   std::unique_ptr<ASTNode> methodBody, bool isStaticMethod = false,
                   const SourceLocation& loc = SourceLocation());

        const std::string& getName() const;
        ValueType getReturnType() const;
        const std::vector<std::pair<std::string, ValueType>>& getParameters() const;
        
        // Safe getter - returns shared_ptr
        [[nodiscard]] std::shared_ptr<ASTNode> getBody() const;
        
        // For code that just needs to read
        [[nodiscard]] ASTNode* getBodyPtr() const noexcept;
        
        bool getIsStatic() const;

        void setName(const std::string& methodName);
        void setReturnType(ValueType retType);
        void setParameters(std::vector<std::pair<std::string, ValueType>> params);
        void setBody(std::shared_ptr<ASTNode> methodBody);
        void setIsStatic(bool isStaticMethod);

        size_t getParameterCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
