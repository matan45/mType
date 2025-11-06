#include "TypeParser.hpp"
#include "../errors/ParseException.hpp"
#include "../ast/GenericType.hpp"
#include "../types/TypeRegistry.hpp"

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

                        // PURE OOP: Primitives are now allowed as generic type arguments!
                        // They will be treated as their Box class equivalents (Int, Float, Bool, String)
                        // No validation needed - all types are now objects

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
}
