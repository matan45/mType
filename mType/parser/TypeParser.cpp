#include "TypeParser.hpp"
#include <cstddef>
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

                    stream.expectGreaterForGeneric(); // consume '>' (handles >> for nested generics)

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

            baseType = currentType;
        }

        // Check for nullable type suffix '?'
        if (stream.check(TokenType::QUESTION))
        {
            // Validate: primitive types cannot be nullable
            if (!baseType->isGenericParameter() && !baseType->isParameterized())
            {
                ValueType vt = baseType->getConcreteType();
                if (vt == ValueType::INT || vt == ValueType::FLOAT ||
                    vt == ValueType::BOOL || vt == ValueType::STRING)
                {
                    throw ParseException(
                        "Primitive types cannot be nullable. Use boxed type instead",
                        stream.location());
                }
                if (vt == ValueType::VOID)
                {
                    throw ParseException("void cannot be nullable", stream.location());
                }
            }
            stream.advance(); // consume '?'
            baseType->setNullable(true);
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
        std::string typeName = std::string(stream.current().stringValue);
        stream.advance();

        // Handle qualified names like ClassName::staticMember
        while (stream.check(TokenType::SCOPE))
        {
            stream.advance();
            if (!stream.check(TokenType::IDENTIFIER))
            {
                throw ParseException("Expected identifier after '::'", stream.location());
            }
            typeName += "::" + std::string(stream.current().stringValue);
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
        std::shared_ptr<ast::GenericType> result;

        // Handle basic types
        if (typeInfo.baseType != ValueType::OBJECT)
        {
            // Simple built-in types (int, float, bool, string, void)
            result = std::make_shared<ast::GenericType>(typeInfo.baseType);
        }
        else
        {
            // Handle object types
            if (!typeInfo.className.empty())
            {
                // For now, treat as regular object type
                // Later we can enhance this to detect generic type parameters
                result = std::make_shared<ast::GenericType>(typeInfo.className);
            }
            else
            {
                // Generic object type
                result = std::make_shared<ast::GenericType>(ValueType::OBJECT);
            }
        }

        if (result && typeInfo.isNullable)
        {
            result->setNullable(true);
        }
        return result;
    }

    TypeInfo TypeParser::convertGenericTypeToTypeInfo(const std::shared_ptr<ast::GenericType>& genericType)
    {
        bool isNullableType = genericType->isNullable();

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

                    TypeInfo result(ValueType::OBJECT, arrayTypeName);
                    result.isNullable = isNullableType;
                    return result;
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
                TypeInfo result(ValueType::OBJECT, fullTypeName);
                result.isNullable = isNullableType;
                return result;
            }

            // Simple generic parameter or class type
            TypeInfo result(ValueType::OBJECT, typeName);
            result.isNullable = isNullableType;
            return result;
        }

        // Handle concrete types
        ValueType baseType = genericType->getConcreteType();

        // Handle non-object types (int, float, bool, string, void)
        if (baseType != ValueType::OBJECT)
        {
            TypeInfo result(baseType);
            result.isNullable = isNullableType;
            return result;
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
            TypeInfo result(ValueType::OBJECT, fullTypeName);
            result.isNullable = isNullableType;
            return result;
        }

        // Simple object type without generic parameters
        TypeInfo result(ValueType::OBJECT, genericType->getBaseTypeName());
        result.isNullable = isNullableType;
        return result;
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

        bool isNullableType = genericType->isNullable();
        types::UnifiedTypePtr result;

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
                    result = types::UnifiedType::arrayOf(elementType);
                    return isNullableType ? result->makeNullable() : result;
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
                result = types::UnifiedType::classType(name, std::move(typeArgs));
                return isNullableType ? result->makeNullable() : result;
            }

            // Simple generic parameter (T, E, etc.)
            result = types::UnifiedType::genericParam(name);
            return isNullableType ? result->makeNullable() : result;
        }

        // Handle concrete types
        ValueType baseType = genericType->getConcreteType();

        switch (baseType)
        {
            case ValueType::INT:
                result = types::UnifiedType::primitive(ValueType::INT); break;
            case ValueType::FLOAT:
                result = types::UnifiedType::primitive(ValueType::FLOAT); break;
            case ValueType::BOOL:
                result = types::UnifiedType::primitive(ValueType::BOOL); break;
            case ValueType::STRING:
                result = types::UnifiedType::primitive(ValueType::STRING); break;
            case ValueType::VOID:
                result = types::UnifiedType::voidType(); break;
            case ValueType::NULL_TYPE:
                result = types::UnifiedType::nullType(); break;
            case ValueType::LAMBDA:
                result = types::UnifiedType::lambdaType(); break;
            case ValueType::ARRAY:
            {
                // Array type with element type
                if (genericType->isParameterized())
                {
                    const auto& typeArgs = genericType->getTypeArguments();
                    if (!typeArgs.empty())
                    {
                        auto elementType = convertGenericTypeToUnifiedType(typeArgs[0]);
                        result = types::UnifiedType::arrayOf(elementType);
                        return isNullableType ? result->makeNullable() : result;
                    }
                }
                // Untyped array
                result = types::UnifiedType::arrayOf(types::UnifiedType::classType("Object"));
                return isNullableType ? result->makeNullable() : result;
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
                    result = types::UnifiedType::classType(className, std::move(typeArgs));
                    return isNullableType ? result->makeNullable() : result;
                }

                result = types::UnifiedType::classType(className);
                return isNullableType ? result->makeNullable() : result;
            }
        }

        return isNullableType ? result->makeNullable() : result;
    }

    std::shared_ptr<ast::GenericType> TypeParser::convertUnifiedTypeToGenericType(
        const types::UnifiedTypePtr& unifiedType)
    {
        if (!unifiedType)
        {
            return nullptr;
        }

        bool isNullableType = unifiedType->isNullable();
        std::shared_ptr<ast::GenericType> result;

        switch (unifiedType->getKind())
        {
            case types::TypeKind::Primitive:
                result = std::make_shared<ast::GenericType>(unifiedType->toValueType());
                break;

            case types::TypeKind::Void:
                result = std::make_shared<ast::GenericType>(ValueType::VOID);
                break;

            case types::TypeKind::Null:
                result = std::make_shared<ast::GenericType>(ValueType::NULL_TYPE);
                break;

            case types::TypeKind::Lambda:
                result = std::make_shared<ast::GenericType>(ValueType::LAMBDA);
                break;

            case types::TypeKind::GenericParameter:
                result = std::make_shared<ast::GenericType>(unifiedType->getName());
                break;

            case types::TypeKind::Array:
            {
                if (!unifiedType->getTypeArguments().empty())
                {
                    auto elementType = convertUnifiedTypeToGenericType(unifiedType->getTypeArguments()[0]);
                    std::vector<std::shared_ptr<ast::GenericType>> args = {elementType};
                    result = std::make_shared<ast::GenericType>("Array", args);
                }
                else
                {
                    result = std::make_shared<ast::GenericType>(ValueType::ARRAY);
                }
                break;
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
                    result = std::make_shared<ast::GenericType>(name, args);
                }
                else
                {
                    result = std::make_shared<ast::GenericType>(name);
                }
                break;
            }

            default:
                result = std::make_shared<ast::GenericType>(ValueType::OBJECT);
                break;
        }

        if (result && isNullableType)
        {
            result->setNullable(true);
        }
        return result;
    }

    types::UnifiedTypePtr TypeParser::convertTypeInfoToUnifiedType(const TypeInfo& typeInfo)
    {
        types::UnifiedTypePtr result;

        switch (typeInfo.baseType)
        {
            case ValueType::INT:
                result = types::UnifiedType::primitive(ValueType::INT); break;
            case ValueType::FLOAT:
                result = types::UnifiedType::primitive(ValueType::FLOAT); break;
            case ValueType::BOOL:
                result = types::UnifiedType::primitive(ValueType::BOOL); break;
            case ValueType::STRING:
                result = types::UnifiedType::primitive(ValueType::STRING); break;
            case ValueType::VOID:
                result = types::UnifiedType::voidType(); break;
            case ValueType::NULL_TYPE:
                result = types::UnifiedType::nullType(); break;
            case ValueType::LAMBDA:
                result = types::UnifiedType::lambdaType(); break;
            case ValueType::ARRAY:
                result = types::UnifiedType::arrayOf(types::UnifiedType::classType("Object")); break;
            case ValueType::OBJECT:
            default:
                if (!typeInfo.className.empty())
                {
                    result = types::UnifiedType::classType(typeInfo.className);
                }
                else
                {
                    result = types::UnifiedType::classType("Object");
                }
                break;
        }

        if (result && typeInfo.isNullable)
        {
            result = result->makeNullable();
        }
        return result;
    }
}
