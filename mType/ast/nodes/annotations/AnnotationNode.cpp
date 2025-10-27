#include "AnnotationNode.hpp"

namespace ast::nodes::annotations
{
    AnnotationNode::AnnotationNode(const std::string& annotationName,
                                   const std::unordered_map<std::string, std::string>& params,
                                   const SourceLocation& loc)
        : ASTNode(loc), name(annotationName), parameters(params)
    {
    }

    const std::string& AnnotationNode::getName() const
    {
        return name;
    }

    void AnnotationNode::setName(const std::string& annotationName)
    {
        name = annotationName;
    }

    const std::unordered_map<std::string, std::string>& AnnotationNode::getParameters() const
    {
        return parameters;
    }

    std::string AnnotationNode::getParameter(const std::string& key, const std::string& defaultValue) const
    {
        auto it = parameters.find(key);
        return it != parameters.end() ? it->second : defaultValue;
    }

    bool AnnotationNode::hasParameter(const std::string& key) const
    {
        return parameters.count(key) > 0;
    }

    void AnnotationNode::setParameter(const std::string& key, const std::string& value)
    {
        parameters[key] = value;
    }

    Value AnnotationNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitAnnotationNode(this);
    }

    std::unique_ptr<ASTNode> AnnotationNode::clone() const
    {
        return std::make_unique<AnnotationNode>(
            name,
            parameters,
            location
        );
    }
}
