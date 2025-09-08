#include "TypeParser.hpp"
#include "../errors/ParseException.hpp"

namespace parser
{
    using namespace errors;

    // Static lookup tables for O(1) type resolution
    const std::unordered_map<TokenType, ValueType> TypeParser::tokenTypeMap = {
        {TokenType::INT, ValueType::INT},
        {TokenType::FLOAT, ValueType::FLOAT},
        {TokenType::BOOL, ValueType::BOOL},
        {TokenType::STRING_TYPE, ValueType::STRING},
        {TokenType::VOID, ValueType::VOID},
        {TokenType::ARRAY, ValueType::ARRAY},
        {TokenType::LIST, ValueType::LIST},
        {TokenType::MAP, ValueType::MAP},
        {TokenType::SET, ValueType::SET},
        {TokenType::QUEUE, ValueType::QUEUE},
        {TokenType::STACK, ValueType::STACK}
    };

    const std::unordered_map<std::string_view, ValueType> TypeParser::stringTypeMap = {
        {"int", ValueType::INT},
        {"float", ValueType::FLOAT},
        {"bool", ValueType::BOOL},
        {"string", ValueType::STRING},
        {"void", ValueType::VOID},
        {"Array", ValueType::ARRAY},
        {"List", ValueType::LIST},
        {"Map", ValueType::MAP},
        {"Set", ValueType::SET},
        {"Queue", ValueType::QUEUE},
        {"Stack", ValueType::STACK}
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

        // Handle dedicated type tokens with O(1) lookup
        auto tokenIt = tokenTypeMap.find(currentType);
        if (tokenIt != tokenTypeMap.end())
        {
            ValueType collectionType = tokenIt->second;
            stream.advance();
            
            // Handle generic parameters for collection types
            if (collectionType == ValueType::ARRAY || collectionType == ValueType::LIST || 
                collectionType == ValueType::SET || collectionType == ValueType::QUEUE || 
                collectionType == ValueType::STACK)
            {
                // These collections expect: CollectionType<ElementType>
                if (stream.check(TokenType::LESS))
                {
                    stream.advance(); // consume '<'
                    TypeInfo elementTypeInfo = parseTypeInfo(stream); // Recursively parse element type
                    stream.expect(TokenType::GREATER); // consume '>'
                    
                    return TypeInfo(collectionType, elementTypeInfo.baseType, 
                                  elementTypeInfo.baseType == ValueType::OBJECT ? elementTypeInfo.className : "");
                }
                else
                {
                    // Collection without generic parameters - use default
                    return TypeInfo(collectionType, ValueType::OBJECT);
                }
            }
            else if (collectionType == ValueType::MAP)
            {
                // Map expects: Map<KeyType, ValueType>
                if (stream.check(TokenType::LESS))
                {
                    stream.advance(); // consume '<'
                    TypeInfo keyTypeInfo = parseTypeInfo(stream); // Parse key type
                    stream.expect(TokenType::COMMA); // consume ','
                    TypeInfo valueTypeInfo = parseTypeInfo(stream); // Parse value type
                    stream.expect(TokenType::GREATER); // consume '>'
                    
                    return TypeInfo(collectionType, keyTypeInfo.baseType, valueTypeInfo.baseType,
                                  keyTypeInfo.baseType == ValueType::OBJECT ? keyTypeInfo.className : "",
                                  valueTypeInfo.baseType == ValueType::OBJECT ? valueTypeInfo.className : "");
                }
                else
                {
                    // Map without generic parameters - use defaults
                    return TypeInfo(collectionType, ValueType::OBJECT, ValueType::OBJECT);
                }
            }
            else
            {
                // Simple type (int, float, bool, string, void)
                return TypeInfo(collectionType);
            }
        }

        // Handle identifier-based types
        if (currentType == TokenType::IDENTIFIER)
        {
            std::string typeName = parseQualifiedName(stream);
            
            // Check if it's a string-based primitive type with O(1) lookup
            auto stringIt = stringTypeMap.find(typeName);
            if (stringIt != stringTypeMap.end())
            {
                ValueType collectionType = stringIt->second;
                
                // Handle generic parameters for collection types
                if (collectionType == ValueType::ARRAY || collectionType == ValueType::LIST || 
                    collectionType == ValueType::SET || collectionType == ValueType::QUEUE || 
                    collectionType == ValueType::STACK)
                {
                    if (stream.check(TokenType::LESS))
                    {
                        stream.advance(); // consume '<'
                        TypeInfo elementTypeInfo = parseTypeInfo(stream); // Parse element type
                        stream.expect(TokenType::GREATER); // consume '>'
                        
                        return TypeInfo(collectionType, elementTypeInfo.baseType,
                                      elementTypeInfo.baseType == ValueType::OBJECT ? elementTypeInfo.className : "");
                    }
                    else
                    {
                        return TypeInfo(collectionType, ValueType::OBJECT);
                    }
                }
                else if (collectionType == ValueType::MAP)
                {
                    if (stream.check(TokenType::LESS))
                    {
                        stream.advance(); // consume '<'
                        TypeInfo keyTypeInfo = parseTypeInfo(stream); // Parse key type
                        stream.expect(TokenType::COMMA); // consume ','
                        TypeInfo valueTypeInfo = parseTypeInfo(stream); // Parse value type
                        stream.expect(TokenType::GREATER); // consume '>'
                        
                        return TypeInfo(collectionType, keyTypeInfo.baseType, valueTypeInfo.baseType,
                                      keyTypeInfo.baseType == ValueType::OBJECT ? keyTypeInfo.className : "",
                                      valueTypeInfo.baseType == ValueType::OBJECT ? valueTypeInfo.className : "");
                    }
                    else
                    {
                        return TypeInfo(collectionType, ValueType::OBJECT, ValueType::OBJECT);
                    }
                }
                else
                {
                    // Simple primitive type
                    return TypeInfo(collectionType);
                }
            }
            else
            {
                // Treat unknown identifier types as custom class types (OBJECT)
                return TypeInfo(ValueType::OBJECT, typeName);
            }
        }

        throw ParseException("Expected type", stream.location());
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
        auto it = stringTypeMap.find(typeName);
        return it != stringTypeMap.end() ? it->second : ValueType::OBJECT;
    }

    std::string TypeParser::parseQualifiedName(TokenStream& stream)
    {
        std::string typeName = stream.current().stringValue;
        stream.advance();

        // Handle qualified names like ClassName::staticMember
        while (stream.check(TokenType::SCOPE))
        {
            stream.advance();
            if (!stream.check(TokenType::IDENTIFIER))
            {
                throw ParseException("Expected identifier after '::'", stream.location());
            }
            typeName += "::" + stream.current().stringValue;
            stream.advance();
        }

        return typeName;
    }
}