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
        TokenType currentType = stream.current().type;

        // Parse base type first
        TypeInfo baseTypeInfo(ValueType::VOID); // Initialize with dummy value

        // Handle dedicated type tokens with O(1) lookup
        auto tokenIt = tokenTypeMap.find(currentType);
        if (tokenIt != tokenTypeMap.end())
        {
            ValueType collectionType = tokenIt->second;
            stream.advance();

            // Simple type (int, float, bool, string, void)
            baseTypeInfo = TypeInfo(tokenIt->second);
        }

        // Handle identifier-based types
        else if (currentType == TokenType::IDENTIFIER)
        {
            std::string typeName = parseQualifiedName(stream);

            // Check if it's a string-based primitive type with O(1) lookup
            auto stringIt = stringTypeMap.find(typeName);
            if (stringIt != stringTypeMap.end())
            {
                ValueType collectionType = stringIt->second;

                // Simple primitive type
                baseTypeInfo = TypeInfo(stringIt->second);
            }
            else
            {
                // Check if this might be a generic type parameter (single letter, capitalized)
                if (typeName.length() == 1 && std::isupper(typeName[0])) {
                    // This looks like a generic type parameter (T, K, V, etc.)
                    // For TypeInfo, we'll treat it as OBJECT with the parameter name
                    baseTypeInfo = TypeInfo(ValueType::OBJECT, typeName);
                }
                else if (stream.check(TokenType::LESS))
                {
                    // This is a generic custom class like Box<int>, Person<T>, etc.
                    // For now, we'll parse the generics but store them as a string
                    // (TypeInfo doesn't have full generic support yet)
                    std::string fullTypeName = typeName + "<";
                    stream.advance(); // consume '<'

                    // Parse type arguments
                    do {
                        TypeInfo argTypeInfo = parseTypeInfo(stream); // Parse type argument
                        fullTypeName += argTypeInfo.toString();

                        if (stream.check(TokenType::COMMA)) {
                            fullTypeName += ", ";
                            stream.advance(); // consume ','
                        } else {
                            break;
                        }
                    } while (true);

                    stream.expect(TokenType::GREATER); // consume '>'
                    fullTypeName += ">";

                    baseTypeInfo = TypeInfo(ValueType::OBJECT, fullTypeName);
                }
                else
                {
                    // Treat unknown identifier types as custom class types (OBJECT)
                    baseTypeInfo = TypeInfo(ValueType::OBJECT, typeName);
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
            // For native arrays, we create a special type that represents native array
            // Arrays are handled as OBJECT type with a special naming convention
            std::string arrayTypeName = baseTypeInfo.toString();
            for (int i = 0; i < arrayDimensions; i++)
            {
                arrayTypeName += "[]";
            }

            // Return as OBJECT type with native array class name
            return TypeInfo(ValueType::OBJECT, arrayTypeName);
        }

        return baseTypeInfo;
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
                if (typeName.length() == 1 && std::isupper(typeName[0])) {
                    // This is a generic type parameter (T, K, V, etc.)
                    baseType = std::make_shared<ast::GenericType>(typeName);
                }
                else if (stream.check(TokenType::LESS)) {
                    stream.advance(); // consume '<'
                    std::vector<std::shared_ptr<ast::GenericType>> typeArgs;

                    // Parse type arguments
                    do {
                        typeArgs.push_back(parseGenericType(stream));

                        if (stream.check(TokenType::COMMA)) {
                            stream.advance(); // consume ','
                        } else {
                            break;
                        }
                    } while (true);

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
        if (it != stringTypeMap.end()) {
            return it->second;
        }

        // Use the global registry for comprehensive type resolution
        try {
            auto& registry = types::getGlobalTypeRegistry();
            return registry.getValueType(std::string(typeName));
        } catch (...) {
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
        
        switch (baseType) {
            case ValueType::INT: result = "int"; break;
            case ValueType::FLOAT: result = "float"; break;
            case ValueType::BOOL: result = "bool"; break;
            case ValueType::STRING: result = "string"; break;
            case ValueType::VOID: result = "void"; break;
            case ValueType::OBJECT:
                result = className.empty() ? "object" : className;
                break;
            default: result = "unknown"; break;
        }
        
        // No collection generic parameters needed anymore
        
        return result;
    }
}