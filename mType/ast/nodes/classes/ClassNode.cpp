#include "ClassNode.hpp"
#include <iostream>

namespace ast::nodes::classes
{
    // Backward compatibility constructor
    ClassNode::ClassNode(const std::string& name, const SourceLocation& loc)
        : ASTNode(loc), className(name), finalClass(false), abstractClass(false), visibility(VisibilityModifier::PUBLIC)
    {
    }

    ClassNode::ClassNode(const std::string& name,
                         const std::vector<GenericTypeParameter>& generics,
                         const std::vector<std::string>& interfaces,
                         const SourceLocation& loc)
        : ASTNode(loc), className(name), genericParameters(generics), implementedInterfaces(interfaces),
          finalClass(false), abstractClass(false), visibility(VisibilityModifier::PUBLIC)
    {
    }

    ClassNode::ClassNode(const std::string& name,
                         const std::vector<GenericTypeParameter>& generics,
                         const std::string& parentClass,
                         const std::vector<std::string>& interfaces,
                         const SourceLocation& loc)
        : ASTNode(loc), className(name), genericParameters(generics),
          parentClassName(parentClass), implementedInterfaces(interfaces), finalClass(false), abstractClass(false),
          visibility(VisibilityModifier::PUBLIC)
    {
    }

    const std::string& ClassNode::getClassName() const
    {
        return className;
    }

    const std::vector<std::unique_ptr<ASTNode>>& ClassNode::getFields() const
    {
        return fields;
    }

    const std::vector<std::unique_ptr<ASTNode>>& ClassNode::getConstructors() const
    {
        return constructors;
    }

    const std::vector<std::unique_ptr<ASTNode>>& ClassNode::getMethods() const
    {
        return methods;
    }

    void ClassNode::setClassName(const std::string& name)
    {
        className = name;
    }

    void ClassNode::addField(std::unique_ptr<ASTNode> field)
    {
        fields.push_back(std::move(field));
    }

    void ClassNode::addConstructor(std::unique_ptr<ASTNode> constructor)
    {
        constructors.push_back(std::move(constructor));
    }

    void ClassNode::addMethod(std::unique_ptr<ASTNode> method)
    {
        methods.push_back(std::move(method));
    }

    size_t ClassNode::getFieldCount() const
    {
        return fields.size();
    }

    size_t ClassNode::getConstructorCount() const
    {
        return constructors.size();
    }

    size_t ClassNode::getMethodCount() const
    {
        return methods.size();
    }

    const std::vector<std::string>& ClassNode::getImplementedInterfaces() const
    {
        return implementedInterfaces;
    }

    const std::vector<GenericTypeParameter>& ClassNode::getGenericParameters() const
    {
        return genericParameters;
    }

    void ClassNode::setGenericParameters(const std::vector<GenericTypeParameter>& generics)
    {
        genericParameters = generics;
    }

    void ClassNode::addGenericParameter(const GenericTypeParameter& param)
    {
        genericParameters.push_back(param);
    }

    size_t ClassNode::getGenericParameterCount() const
    {
        return genericParameters.size();
    }

    bool ClassNode::isGeneric() const
    {
        return !genericParameters.empty();
    }

    std::string ClassNode::getFullClassName() const
    {
        std::string fullName = className;
        if (!genericParameters.empty())
        {
            fullName += "<";
            for (size_t i = 0; i < genericParameters.size(); ++i)
            {
                if (i > 0) fullName += ", ";
                fullName += genericParameters[i].toString();
            }
            fullName += ">";
        }
        return fullName;
    }

    const std::string& ClassNode::getParentClassName() const
    {
        return parentClassName;
    }

    void ClassNode::setParentClassName(const std::string& parent)
    {
        parentClassName = parent;
    }

    bool ClassNode::hasParentClass() const
    {
        return !parentClassName.empty();
    }

    bool ClassNode::isFinal() const
    {
        return finalClass;
    }

    void ClassNode::setFinal(bool isFinal)
    {
        finalClass = isFinal;
    }

    bool ClassNode::isAbstract() const
    {
        return abstractClass;
    }

    void ClassNode::setAbstract(bool isAbstract)
    {
        abstractClass = isAbstract;
    }

    VisibilityModifier ClassNode::getVisibility() const
    {
        return visibility;
    }

    void ClassNode::setVisibility(VisibilityModifier vis)
    {
        visibility = vis;
    }

    const std::vector<std::shared_ptr<annotations::AnnotationNode>>& ClassNode::getAnnotations() const
    {
        return annotations;
    }

    void ClassNode::addAnnotation(std::shared_ptr<annotations::AnnotationNode> annotation)
    {
        annotations.push_back(annotation);
    }

    bool ClassNode::hasAnnotation(const std::string& annotationName) const
    {
        for (const auto& annotation : annotations)
        {
            if (annotation->getName() == annotationName)
            {
                return true;
            }
        }
        return false;
    }

    std::shared_ptr<annotations::AnnotationNode> ClassNode::getAnnotation(const std::string& annotationName) const
    {
        for (const auto& annotation : annotations)
        {
            if (annotation->getName() == annotationName)
            {
                return annotation;
            }
        }
        return nullptr;
    }

    Value ClassNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitClassNode(this);
    }

    std::unique_ptr<ASTNode> ClassNode::clone() const
    {
        // Create cloned class with inheritance constructor
        auto clonedClass = std::make_unique<ClassNode>(
            className,
            genericParameters,
            parentClassName,
            implementedInterfaces,
            location
        );

        // Clone fields
        for (const auto& field : fields)
        {
            if (field)
            {
                clonedClass->addField(field->clone());
            }
        }

        // Clone constructors
        for (const auto& constructor : constructors)
        {
            if (constructor)
            {
                clonedClass->addConstructor(constructor->clone());
            }
        }

        // Clone methods
        for (const auto& method : methods)
        {
            if (method)
            {
                clonedClass->addMethod(method->clone());
            }
        }

        // Set other attributes
        clonedClass->setFinal(finalClass);
        clonedClass->setAbstract(abstractClass);
        clonedClass->setVisibility(visibility);

        // Clone annotations
        for (const auto& annotation : annotations)
        {
            if (annotation)
            {
                // Safely reconstruct annotation using make_shared to avoid unsafe cast
                auto clonedAnnotation = std::make_shared<annotations::AnnotationNode>(
                    annotation->getName(),
                    annotation->getParameters(),
                    annotation->getLocation()
                );
                clonedClass->addAnnotation(clonedAnnotation);
            }
        }

        return clonedClass;
    }
}
