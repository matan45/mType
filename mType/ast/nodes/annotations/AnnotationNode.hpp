#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <memory>
#include <unordered_map>

namespace ast::nodes::annotations
{
    class AnnotationNode : public ASTNode
    {
    private:
        std::string name;
        std::unordered_map<std::string, std::string> parameters;

    public:
        explicit AnnotationNode(const std::string& annotationName,
                                const std::unordered_map<std::string, std::string>& params = {},
                                const SourceLocation& loc = SourceLocation());

        const std::string& getName() const;
        void setName(const std::string& annotationName);

        const std::unordered_map<std::string, std::string>& getParameters() const;
        std::string getParameter(const std::string& key, const std::string& defaultValue = "") const;
        bool hasParameter(const std::string& key) const;
        void setParameter(const std::string& key, const std::string& value);

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
