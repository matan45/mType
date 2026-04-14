#include "FieldNode.hpp"

namespace ast::nodes::classes
{
    // NEW: Primary constructor with GenericType support
    FieldNode::FieldNode(const std::string& fieldName,
                         std::shared_ptr<GenericType> fieldType,
                         std::unique_ptr<ASTNode> initValue,
                         bool isStaticField, bool isFinalField,
                         AccessModifier modifier,
                         const SourceLocation& loc)
        : ASTNode(loc), name(fieldName), type(fieldType), initialValue(std::move(initValue)),
          isStatic(isStaticField), isFinal(isFinalField), accessModifier(modifier)
    {
    }

    // Backward compatibility constructor with ValueType
    FieldNode::FieldNode(const std::string& fieldName, ValueType fieldType,
                         std::unique_ptr<ASTNode> initValue,
                         bool isStaticField, bool isFinalField,
                         AccessModifier modifier,
                         const SourceLocation& loc)
        : ASTNode(loc), name(fieldName), initialValue(std::move(initValue)),
          isStatic(isStaticField), isFinal(isFinalField), accessModifier(modifier)
    {
        // Convert ValueType to GenericType
        type = std::make_shared<GenericType>(fieldType);
    }

    const std::string& FieldNode::getName() const
    {
        return name;
    }

    std::shared_ptr<GenericType> FieldNode::getGenericType() const
    {
        return type;
    }

    // Legacy getter for backward compatibility
    ValueType FieldNode::getType() const
    {
        if (type && !type->isGenericParameter())
        {
            return type->getConcreteType();
        }
        return ValueType::OBJECT; // Default for generic parameters
    }

    ASTNode* FieldNode::getInitialValue() const
    {
        return initialValue.get();
    }

    bool FieldNode::getIsStatic() const
    {
        return isStatic;
    }

    bool FieldNode::getIsFinal() const
    {
        return isFinal;
    }

    void FieldNode::setName(const std::string& fieldName)
    {
        name = fieldName;
    }

    void FieldNode::setGenericType(std::shared_ptr<GenericType> fieldType)
    {
        type = fieldType;
    }

    // Legacy setter for backward compatibility
    void FieldNode::setType(ValueType fieldType)
    {
        type = std::make_shared<GenericType>(fieldType);
    }

    void FieldNode::setInitialValue(std::unique_ptr<ASTNode> initValue)
    {
        initialValue = std::move(initValue);
    }

    void FieldNode::setIsStatic(bool isStaticField)
    {
        isStatic = isStaticField;
    }

    void FieldNode::setIsFinal(bool isFinalField)
    {
        isFinal = isFinalField;
    }

    AccessModifier FieldNode::getAccessModifier() const
    {
        return accessModifier;
    }

    void FieldNode::setAccessModifier(AccessModifier modifier)
    {
        accessModifier = modifier;
    }

    bool FieldNode::hasInitialValue() const
    {
        return initialValue != nullptr;
    }

    Value FieldNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitFieldNode(this);
    }

    std::unique_ptr<ASTNode> FieldNode::clone() const
    {
        // Clone initial value
        std::unique_ptr<ASTNode> clonedInitValue = initialValue ? initialValue->clone() : nullptr;

        // Clone the GenericType
        std::shared_ptr<GenericType> clonedType = type ? std::make_shared<GenericType>(*type) : nullptr;

        // Create cloned field
        auto clonedField = std::make_unique<FieldNode>(
            name,
            clonedType,
            std::move(clonedInitValue),
            isStatic,
            isFinal,
            accessModifier,
            location
        );

        // MYT-108: clone annotations via typed-parameter path so CLASS_ARRAY etc. survive.
        for (const auto& annotation : annotations)
        {
            if (!annotation) continue;
            auto cloned = std::make_shared<annotations::AnnotationNode>(
                annotation->getName(), annotation->getLocation());
            for (const auto& key : annotation->getKeyOrder())
            {
                if (auto* v = annotation->getTypedParameter(key))
                {
                    cloned->setTypedParameter(key, *v);
                }
            }
            clonedField->addAnnotation(cloned);
        }

        return clonedField;
    }
}
