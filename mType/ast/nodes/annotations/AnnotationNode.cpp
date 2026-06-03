#include "AnnotationNode.hpp"
#include <algorithm>

namespace ast::nodes::annotations
{
    AnnotationNode::AnnotationNode(const std::string& annotationName,
                                   const SourceLocation& loc)
        : ASTNode(loc), name(annotationName)
    {
    }

    AnnotationNode::AnnotationNode(const std::string& annotationName,
                                   const std::unordered_map<std::string, std::string>& params,
                                   const SourceLocation& loc)
        : ASTNode(loc), name(annotationName)
    {
        for (const auto& [k, v] : params)
        {
            setParameter(k, v);
        }
    }

    const std::string& AnnotationNode::getName() const { return name; }
    void AnnotationNode::setName(const std::string& annotationName) { name = annotationName; }

    void AnnotationNode::setTypedParameter(const std::string& key, TypedAnnotationValue value)
    {
        if (typedParameters.find(key) == typedParameters.end())
        {
            keyOrder.push_back(key);
        }
        typedParameters.insert_or_assign(key, std::move(value));
    }

    const TypedAnnotationValue* AnnotationNode::getTypedParameter(const std::string& key) const
    {
        auto it = typedParameters.find(key);
        return it != typedParameters.end() ? &it->second : nullptr;
    }

    const std::unordered_map<std::string, TypedAnnotationValue>& AnnotationNode::getTypedParameters() const
    {
        return typedParameters;
    }

    bool AnnotationNode::removeTypedParameter(const std::string& key)
    {
        auto it = typedParameters.find(key);
        if (it == typedParameters.end()) return false;
        typedParameters.erase(it);
        auto ki = std::find(keyOrder.begin(), keyOrder.end(), key);
        if (ki != keyOrder.end()) keyOrder.erase(ki);
        return true;
    }

    void AnnotationNode::setDeferredExpression(const std::string& key, std::shared_ptr<ASTNode> expr)
    {
        deferredExpressions.insert_or_assign(key, std::move(expr));
    }

    const std::unordered_map<std::string, std::shared_ptr<ASTNode>>& AnnotationNode::getDeferredExpressions() const
    {
        return deferredExpressions;
    }

    bool AnnotationNode::hasDeferredExpressions() const
    {
        return !deferredExpressions.empty();
    }

    void AnnotationNode::clearDeferredExpressions()
    {
        deferredExpressions.clear();
    }

    std::unordered_map<std::string, std::string> AnnotationNode::getParameters() const
    {
        std::unordered_map<std::string, std::string> out;
        for (const auto& [k, v] : typedParameters)
        {
            out[k] = v.toDisplayString();
        }
        return out;
    }

    std::string AnnotationNode::getParameter(const std::string& key, const std::string& defaultValue) const
    {
        auto it = typedParameters.find(key);
        return it != typedParameters.end() ? it->second.toDisplayString() : defaultValue;
    }

    bool AnnotationNode::hasParameter(const std::string& key) const
    {
        return typedParameters.count(key) > 0;
    }

    void AnnotationNode::setParameter(const std::string& key, const std::string& value)
    {
        setTypedParameter(key, TypedAnnotationValue::makeString(value));
    }

    Value AnnotationNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitAnnotationNode(this);
    }

    std::unique_ptr<ASTNode> AnnotationNode::clone() const
    {
        auto copy = std::make_unique<AnnotationNode>(name, location);
        for (const auto& key : keyOrder)
        {
            auto it = typedParameters.find(key);
            if (it != typedParameters.end())
            {
                copy->setTypedParameter(key, it->second);
            }
        }
        // MYT-376: deep-copy any not-yet-folded deferred expressions so a clone
        // taken before the resolver runs stays self-consistent.
        for (const auto& [key, expr] : deferredExpressions)
        {
            copy->setDeferredExpression(key, expr ? std::shared_ptr<ASTNode>(expr->clone()) : nullptr);
        }
        return copy;
    }
}
