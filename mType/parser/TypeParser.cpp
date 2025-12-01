#include "TypeParser.hpp"
#include "../errors/ParseException.hpp"
#include "../ast/GenericType.hpp"
#include "../types/TypeRegistry.hpp"
#include "../types/UnifiedType.hpp"

namespace parser
{
    using namespace errors;

    // Static lookup tables for O(1) type resolution
    const std::unordered_map<TokenType, ValueType> TypeParser::tokenTypeMap = {
        {TokenType::INT, ValueType::INT},
        {TokenType::FLOAT, ValueType::FLOAT},
        {TokenType::BOOL, ValueType::BOOL},
        {TokenType::STRING_TYPE, ValueType::STRING},
        {TokenType::VOID, ValueType::VOID}
    };

    const std::unordered_map<std::string_view, ValueType> TypeParser::stringTypeMap = {
        {"int", ValueType::INT},
        {"float", ValueType::FLOAT},
        {"bool", ValueType::BOOL},
        {"string", ValueType::STRING},
        {"void", ValueType::VOID},
        {"array", ValueType::ARRAY},
        {"null", ValueType::NULL_TYPE},
        {"lambda", ValueType::LAMBDA}
    };

    const std::unordered_set<TokenType> TypeParser::assignmentOperators = {
        TokenType::ASSIGN,
        TokenType::PLUS_ASSIGN,
        TokenType::MINUS_ASSIGN,
        TokenType::MULTIPLY_ASSIGN,
        TokenType::DIVIDE_ASSIGN,
        TokenType::MODULO_ASSIGN
    };

    ValueType TypeParser::parseType(TokenStream& stream)
    {
        // Use the new parseTypeInfo method and extract just the base type
        TypeInfo typeInfo = parseTypeInfo(stream);
        return typeInfo.baseType;
    }

    TypeInfo TypeParser::parseTypeInfo(TokenStream& stream)
    {
        // Delegate to parseGenericType and convert the result
        // This eliminates ~100 lines of duplicate parsing logic
        auto genericType = parseGenericType(stream);
        return convertGenericTypeToTypeInfo(genericType);
    }

    std::shared_ptr<ast::GenericType> TypeParser::parseGenericType(TokenStream& stream)
    {
        TokenType currentType = stream.current().type;

        // Parse base type first
        std::shared_ptr<ast::GenericType> baseType;

        // Handle dedicated type tokens with O(1) lookup
        auto tokenIt = tokenTypeMap.find(currentType);
        if (tokenIt != tokenTypeMap.end())
        {
            ValueType valueType = tokenIt->second;
            stream.advance();

            // Simple type (int, float, bool, string, void)
            baseType = std::make_shared<ast::GenericType>(valueType);
        }

        // Handle identifier-based types
        else if (currentType == TokenType::IDENTIFIER)
        {
            std::string typeName = parseQualifiedName(stream);

            // Check if it's a string-based primitive type with O(1) lookup
            auto stringIt = stringTypeMap.find(typeName);
            if (stringIt != stringTypeMap.end())
            {
                ValueType valueType = stringIt->second;

                // Simple primitive type
                baseType = std::make_shared<ast::GenericType>(valueType);
            }
            else
            {
                // Check if this might be a generic type parameter (single letter, capitalized)
                if (typeName.length() == 1 && std::isupper(typeName[0]))
                {
                    // This is a generic type parameter (T, K, V, etc.)
                    baseType = std::make_shared<ast::GenericType>(typeName);
                }
                else if (stream.check(TokenType::LESS))
                {
                    stream.advance(); // consume '<'
                    std::vector<std::shared_ptr<ast::GenericType>> typeArgs;

                    // Parse type arguments
                    do
                    {
                        auto typeArg = parseGenericType(stream);

                        // Validate: Reject primitive types in generic type arguments
                        // Users must use boxed class versions (Int, Bool, String, Float)
                        if (!typeArg->isGenericParameter() && !typeArg->isParameterized())
                        {
                            ValueType argType = typeArg->getConcreteType();
                            if (argType == ValueType::INT || argType == ValueType::BOOL ||
                                argType == ValueType::STRING || argType == ValueType::FLOAT)
                            {
                                std::string primitiveTypeName;
                                std::string boxedTypeName;

                                switch (argType)
                                {
                                    case ValueType::INT:
                                        primitiveTypeName = "int";
                                        boxedTypeName = "Int";
                                        break;
                                    case ValueType::BOOL:
                                        primitiveTypeName = "bool";
                                        boxedTypeName = "Bool";
                                        break;
                                    case ValueType::STRING:
                                        primitiveTypeName = "string";
                                        boxedTypeName = "String";
                                        break;
                                    case ValueType::FLOAT:
                                        primitiveTypeName = "float";
                                        boxedTypeName = "Float";
                                        break;
                                    default:
                                        break;
                                }

                                throw ParseException(
                                    "Primitive type '" + primitiveTypeName + "' cannot be used as generic type argument. Use boxed type '" + boxedTypeName + "' instead",
                                    stream.location()
                                );
                            }
                        }

                        typeArgs.push_back(typeArg);

                        if (stream.check(TokenType::COMMA))
                        {
                            stream.advance(); // consume ','
                        }
                        else
                        {
                            break;
                        }
                    }
                    while (true);

                    stream.expect(TokenType::GREATER); // consume '>'

                    // Create generic type for custom class with type arguments
                    baseType = std::make_shared<ast::GenericType>(typeName, typeArgs);
                }
                else
                {
                    // Treat as custom class type without type arguments
                    baseType = std::make_shared<ast::GenericType>(typeName);
                }
            }
        }
        else
        {
            throw ParseException("Expected type", stream.location());
        }

        // Now check for array brackets: int[], string[][], T[], etc.
        int arrayDimensions = 0;
        while (stream.check(TokenType::LBRACKET))
        {
            stream.advance(); // consume '['
            stream.expect(TokenType::RBRACKET); // consume ']'
            arrayDimensions++;
        }

        // Handle array type declarations (T[], int[], etc.)
        if (arrayDimensions > 0)
        {
            auto currentType = baseType;

            // Create array type for each dimension
            for (int i = 0; i < arrayDimensions; i++)
            {
                // For generic types, we create a special array type
                std::vector<std::shared_ptr<ast::GenericType>> typeArgs = {currentType};
                currentType = std::make_shared<ast::GenericType>("Array", typeArgs);
            }

            return currentType;
        }

        return baseType;
    }

    std::pair<ValueType, std::string> TypeParser::parseTypeWithClassName(TokenStream& stream)
    {
        TokenType currentType = stream.current().type;

        // Handle dedicated type tokens
        auto tokenIt = tokenTypeMap.find(currentType);
        if (tokenIt != tokenTypeMap.end())
        {
            stream.advance();
            return {tokenIt->second, ""};
        }

        // Handle identifier-based types
        if (currentType == TokenType::IDENTIFIER)
        {
            std::string typeName = parseQualifiedName(stream);

            // Check if it's a string-based primitive type
            auto stringIt = stringTypeMap.find(typeName);
            if (stringIt != stringTypeMap.end())
            {
                return {stringIt->second, ""};
            }
            else
            {
                // Return OBJECT type with the actual class name
                return {ValueType::OBJECT, typeName};
            }
        }

        throw ParseException("Expected type", stream.location());
    }

    bool TypeParser::isAssignmentOperator(TokenType tokenType) noexcept
    {
        return assignmentOperators.find(tokenType) != assignmentOperators.end();
    }

    ValueType TypeParser::tokenToValueType(TokenType tokenType) noexcept
    {
        auto it = tokenTypeMap.find(tokenType);
        return it != tokenTypeMap.end() ? it->second : ValueType::OBJECT;
    }

    ValueType TypeParser::stringToValueType(std::string_view typeName) noexcept
    {
        // First check the static map for primitive types (fast path)
        auto it = stringTypeMap.find(typeName);
        if (it != stringTypeMap.end())
        {
            return it->second;
        }

        // Use the global registry for comprehensive type resolution
        try
        {
            auto& registry = types::getGlobalTypeRegistry();
            return registry.getValueType(std::string(typeName));
        }
        catch (...)
        {
            // If registry fails, fallback to OBJECT (maintains backward compatibility)
            return ValueType::OBJECT;
        }
    }

    std::string TypeParser::parseQualifiedName(TokenStream& stream)
    {
        std::string typeName = stream.current().stringValue.getString();
        stream.advance();

        // Handle qualified names like ClassName::staticMember
        while (stream.check(TokenType::SCOPE))
        {
            stream.advance();
            if (!stream.check(TokenType::IDENTIFIER))
            {
                throw ParseException("Expected identifier after '::'", stream.location());
            }
            typeName += "::" + stream.current().stringValue.getString();
            stream.advance();
        }

        return typeName;
    }

    std::string TypeInfo::toString() const
    {
        // Convert ValueType to string
        std::string result;

        switch (baseType)
        {
        case ValueType::INT: result = "int";
            break;
        case ValueType::FLOAT: result = "float";
            break;
        case ValueType::BOOL: result = "bool";
            break;
        case ValueType::STRING: result = "string";
            break;
        case ValueType::VOID: result = "void";
            break;
        case ValueType::OBJECT:
            result = className.empty() ? "object" : className;
            break;
        default: result = "unknown";
            break;
        }

        // No collection generic parameters needed anymore

        return result;
    }

    TypeInfo TypeParser::createTypeInfoFromClassName(const std::string& className)
    {
        // Handle basic types
        if (className == "int") return TypeInfo(ValueType::INT);
        if (className == "float") return TypeInfo(ValueType::FLOAT);
        if (className == "bool") return TypeInfo(ValueType::BOOL);
        if (className == "string") return TypeInfo(ValueType::STRING);
        if (className == "void") return TypeInfo(ValueType::VOID);

        // For complex generic types like "Array<int>", extract and parse
        size_t anglePos = className.find('<');
        if (anglePos != std::string::npos)
        {
            // Custom generic class - treat as object
            return TypeInfo(ValueType::OBJECT, className);
        }

        // Default: treat as custom class (OBJECT type)
        return TypeInfo(ValueType::OBJECT, className);
    }

    std::shared_ptr<ast::GenericType> TypeParser::convertTypeInfoToGenericType(const TypeInfo& typeInfo)
    {
        // Handle basic types
        if (typeInfo.baseType != ValueType::OBJECT)
        {
            // Simple built-in types (int, float, bool, string, void)
            return std::make_shared<ast::GenericType>(typeInfo.baseType);
        }
        else
        {
            // Handle object types
            if (!typeInfo.className.empty())
            {
                // For now, treat as regular object type
                // Later we can enhance this to detect generic type parameters
                return std::make_shared<ast::GenericType>(typeInfo.className);
            }
            else
            {
                // Generic object type
                return std::make_shared<ast::GenericType>(ValueType::OBJECT);
            }
        }
    }

    TypeInfo TypeParser::convertGenericTypeToTypeInfo(const std::shared_ptr<ast::GenericType>& genericType)
    {
        // Handle generic type parameters (T, K, V, etc.)
        if (genericType->isGenericParameter())
        {
            std::string typeName = genericType->getGenericName();

            // Check if this is an array type (Array<T>)
            if (typeName == "Array" && genericType->isParameterized())
            {
                // Get the element type and convert recursively
                const auto& typeArgs = genericType->getTypeArguments();
                if (!typeArgs.empty())
                {
                    auto elementType = typeArgs[0];
                    TypeInfo elementTypeInfo = convertGenericTypeToTypeInfo(elementType);

                    // Build array type name with brackets
                    // The recursive call already handles nested Array types,
                    // so we just need to add one [] for this level
                    std::string arrayTypeName = elementTypeInfo.toString() + "[]";

                    return TypeInfo(ValueType::OBJECT, arrayTypeName);
                }
            }

            // Handle generic classes with type arguments (Box<T>, Map<K, V>, etc.)
            if (genericType->isParameterized())
            {
                std::string fullTypeName = typeName + "<";

                const auto& typeArgs = genericType->getTypeArguments();
                for (size_t i = 0; i < typeArgs.size(); ++i)
                {
                    if (i > 0) fullTypeName += ", ";
                    TypeInfo argTypeInfo = convertGenericTypeToTypeInfo(typeArgs[i]);
                    fullTypeName += argTypeInfo.toString();
                }

                fullTypeName += ">";
                return TypeInfo(ValueType::OBJECT, fullTypeName);
            }

            // Simple generic parameter or class type
            return TypeInfo(ValueType::OBJECT, typeName);
        }

        // Handle concrete types
        ValueType baseType = genericType->getConcreteType();

        // Handle non-object types (int, float, bool, string, void)
        if (baseType != ValueType::OBJECT)
        {
            return TypeInfo(baseType);
        }

        // Handle object types with type arguments (shouldn't happen for OBJECT base type, but handle for completeness)
        if (genericType->isParameterized())
        {
            std::string className = genericType->getBaseTypeName();
            std::string fullTypeName = className + "<";

            const auto& typeArgs = genericType->getTypeArguments();
            for (size_t i = 0; i < typeArgs.size(); ++i)
            {
                if (i > 0) fullTypeName += ", ";
                TypeInfo argTypeInfo = convertGenericTypeToTypeInfo(typeArgs[i]);
                fullTypeName += argTypeInfo.toString();
            }

            fullTypeName += ">";
            return TypeInfo(ValueType::OBJECT, fullTypeName);
        }

        // Simple object type without generic parameters
        return TypeInfo(ValueType::OBJECT, genericType->getBaseTypeName());
    }

    // ============== UnifiedType Support Implementation ==============

    types::UnifiedTypePtr TypeParser::parseUnifiedType(TokenStream& stream)
    {
        // Delegate to parseGenericType and convert
        auto genericType = parseGenericType(stream);
        return convertGenericTypeToUnifiedType(genericType);
    }

    types::UnifiedTypePtr TypeParser::convertGenericTypeToUnifiedType(
        const std::shared_ptr<ast::GenericType>& genericType)
    {
        if (!genericType)
        {
            return nullptr;
        }

        // Handle generic type parameters (T, K, V, etc.)
        if (genericType->isGenericParameter())
        {
            std::string name = genericType->getGenericName();

            // Check if this is Array<T> (special case for arrays)
            if (name == "Array" && genericType->isParameterized())
            {
                const auto& typeArgs = genericType->getTypeArguments();
                if (!typeArgs.empty())
                {
                    auto elementType = convertGenericTypeToUnifiedType(typeArgs[0]);
                    return types::UnifiedType::arrayOf(elementType);
                }
            }

            // Check if it's parameterized (e.g., Container<T>)
            if (genericType->isParameterized())
            {
                std::vector<types::UnifiedTypePtr> typeArgs;
                for (const auto& arg : genericType->getTypeArguments())
                {
                    typeArgs.push_back(convertGenericTypeToUnifiedType(arg));
                }
                return types::UnifiedType::classType(name, std::move(typeArgs));
            }

            // Simple generic parameter (T, E, etc.)
            return types::UnifiedType::genericParam(name);
        }

        // Handle concrete types
        ValueType baseType = genericType->getConcreteType();

        switch (baseType)
        {
            case ValueType::INT:
                return types::UnifiedType::primitive(ValueType::INT);
            case ValueType::FLOAT:
                return types::UnifiedType::primitive(ValueType::FLOAT);
            case ValueType::BOOL:
                return types::UnifiedType::primitive(ValueType::BOOL);
            case ValueType::STRING:
                return types::UnifiedType::primitive(ValueType::STRING);
            case ValueType::VOID:
                return types::UnifiedType::voidType();
            case ValueType::NULL_TYPE:
                return types::UnifiedType::nullType();
            case ValueType::LAMBDA:
                return types::UnifiedType::lambdaType();
            case ValueType::ARRAY:
            {
                // Array type with element type
                if (genericType->isParameterized())
                {
                    const auto& typeArgs = genericType->getTypeArguments();
                    if (!typeArgs.empty())
                    {
                        auto elementType = convertGenericTypeToUnifiedType(typeArgs[0]);
                        return types::UnifiedType::arrayOf(elementType);
                    }
                }
                // Untyped array
                return types::UnifiedType::arrayOf(types::UnifiedType::classType("Object"));
            }
            case ValueType::OBJECT:
            default:
            {
                std::string className = genericType->getBaseTypeName();

                // Handle parameterized object types
                if (genericType->isParameterized())
                {
                    std::vector<types::UnifiedTypePtr> typeArgs;
                    for (const auto& arg : genericType->getTypeArguments())
                    {
                        typeArgs.push_back(convertGenericTypeToUnifiedType(arg));
                    }
                    return types::UnifiedType::classType(className, std::move(typeArgs));
                }

                return types::UnifiedType::classType(className);
            }
        }
    }

    std::shared_ptr<ast::GenericType> TypeParser::convertUnifiedTypeToGenericType(
        const types::UnifiedTypePtr& unifiedType)
    {
        if (!unifiedType)
        {
            return nullptr;
        }

        switch (unifiedType->getKind())
        {
            case types::TypeKind::Primitive:
                return std::make_shared<ast::GenericType>(unifiedType->toValueType());

            case types::TypeKind::Void:
                return std::make_shared<ast::GenericType>(ValueType::VOID);

            case types::TypeKind::Null:
                return std::make_shared<ast::GenericType>(ValueType::NULL_TYPE);

            case types::TypeKind::Lambda:
                return std::make_shared<ast::GenericType>(ValueType::LAMBDA);

            case types::TypeKind::GenericParameter:
                return std::make_shared<ast::GenericType>(unifiedType->getName());

            case types::TypeKind::Array:
            {
                if (!unifiedType->getTypeArguments().empty())
                {
                    auto elementType = convertUnifiedTypeToGenericType(unifiedType->getTypeArguments()[0]);
                    std::vector<std::shared_ptr<ast::GenericType>> args = {elementType};
                    return std::make_shared<ast::GenericType>("Array", args);
                }
                return std::make_shared<ast::GenericType>(ValueType::ARRAY);
            }

            case types::TypeKind::Class:
            case types::TypeKind::Interface:
            {
                std::string name = unifiedType->getName();
                if (unifiedType->isParameterized())
                {
                    std::vector<std::shared_ptr<ast::GenericType>> args;
                    for (const auto& arg : unifiedType->getTypeArguments())
                    {
                        args.push_back(convertUnifiedTypeToGenericType(arg));
                    }
                    return std::make_shared<ast::GenericType>(name, args);
                }
                return std::make_shared<ast::GenericType>(name);
            }

            default:
                return std::make_shared<ast::GenericType>(ValueType::OBJECT);
        }
    }

    types::UnifiedTypePtr TypeParser::convertTypeInfoToUnifiedType(const TypeInfo& typeInfo)
    {
        switch (typeInfo.baseType)
        {
            case ValueType::INT:
                return types::UnifiedType::primitive(ValueType::INT);
            case ValueType::FLOAT:
                return types::UnifiedType::primitive(ValueType::FLOAT);
            case ValueType::BOOL:
                return types::UnifiedType::primitive(ValueType::BOOL);
            case ValueType::STRING:
                return types::UnifiedType::primitive(ValueType::STRING);
            case ValueType::VOID:
                return types::UnifiedType::voidType();
            case ValueType::NULL_TYPE:
                return types::UnifiedType::nullType();
            case ValueType::LAMBDA:
                return types::UnifiedType::lambdaType();
            case ValueType::ARRAY:
                return types::UnifiedType::arrayOf(types::UnifiedType::classType("Object"));
            case ValueType::OBJECT:
            default:
                if (!typeInfo.className.empty())
                {
                    return types::UnifiedType::classType(typeInfo.className);
                }
                return types::UnifiedType::classType("Object");
        }
    }
}
