#include "ArrayTypeNode.hpp"

namespace ast::nodes::expressions
{
    ArrayTypeNode::ArrayTypeNode(const parser::TypeInfo& elemTypeInfo,
                                int dims,
                                const SourceLocation& loc)
        : ASTNode(loc), elementTypeInfo(elemTypeInfo), dimensions(dims), isGenericParameter(false) {}

    ArrayTypeNode::ArrayTypeNode(const std::string& genericTypeName,
                                int dims,
                                const SourceLocation& loc)
        : ASTNode(loc), elementTypeInfo(parser::TypeInfo(ValueType::OBJECT, genericTypeName)),
          dimensions(dims), isGenericParameter(true), genericTypeName(genericTypeName) {}

    const parser::TypeInfo& ArrayTypeNode::getElementTypeInfo() const
    {
        return elementTypeInfo;
    }

    ValueType ArrayTypeNode::getElementType() const
    {
        return elementTypeInfo.baseType;
    }

    int ArrayTypeNode::getDimensions() const
    {
        return dimensions;
    }

    bool ArrayTypeNode::isGeneric() const
    {
        return isGenericParameter;
    }

    std::string ArrayTypeNode::getGenericTypeName() const
    {
        return genericTypeName;
    }

    std::string ArrayTypeNode::toString() const
    {
        std::string result;

        if (isGenericParameter) {
            result = genericTypeName;
        } else {
            // Convert ValueType to string
            switch (elementTypeInfo.baseType) {
                case ValueType::INT: result = "int"; break;
                case ValueType::FLOAT: result = "float"; break;
                case ValueType::BOOL: result = "bool"; break;
                case ValueType::STRING: result = "string"; break;
                case ValueType::OBJECT: result = elementTypeInfo.className; break;
                default: result = "unknown"; break;
            }
        }

        // Add brackets for each dimension
        for (int i = 0; i < dimensions; i++) {
            result += "[]";
        }

        return result;
    }

    parser::TypeInfo ArrayTypeNode::toCollectionType() const
    {
        // Convert int[] to Array<int>, string[][] to Array<Array<string>>, etc.
        parser::TypeInfo result = elementTypeInfo;

        // Wrap in Array<> for each dimension
        for (int i = 0; i < dimensions; i++) {
            if (result.baseType == ValueType::OBJECT && !result.className.empty()) {
                // Object type with class name
                result = parser::TypeInfo(ValueType::ARRAY,
                                        std::make_shared<parser::TypeInfo>(result));
            } else {
                // Primitive type
                result = parser::TypeInfo(ValueType::ARRAY,
                                        std::make_shared<parser::TypeInfo>(result));
            }
        }

        return result;
    }

    Value ArrayTypeNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitArrayTypeNode(this);
    }
}