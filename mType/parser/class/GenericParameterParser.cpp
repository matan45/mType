#include "GenericParameterParser.hpp"
#include "../utilities/ParserUtils.hpp"
#include "../../errors/ParseException.hpp"

namespace parser
{
    using namespace token;
    using namespace errors;

    GenericParameterParser::GenericParameterParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
    }

    std::unique_ptr<ASTNode> GenericParameterParser::parse()
    {
        // This parser is primarily a utility parser and doesn't create standalone AST nodes
        throw ParseException("GenericParameterParser is a utility parser", tokenStream.current().location);
    }

    bool GenericParameterParser::canParse(const TokenStream& stream) const
    {
        return stream.check(TokenType::LESS);
    }

    std::string GenericParameterParser::parseGenericParameters()
    {
        std::string result;

        // Parse first parameter
        result += parseGenericParameter();

        // Parse additional parameters separated by commas
        while (tokenStream.check(TokenType::COMMA))
        {
            result += ", ";
            tokenStream.advance(); // consume ','
            result += parseGenericParameter();
        }

        return result;
    }

    std::string GenericParameterParser::parseGenericParameter()
    {
        // Handle type name (could be identifier or built-in type)
        if (tokenStream.current().type == TokenType::IDENTIFIER ||
            tokenStream.current().type == TokenType::INT ||
            tokenStream.current().type == TokenType::FLOAT ||
            tokenStream.current().type == TokenType::BOOL ||
            tokenStream.current().type == TokenType::STRING_TYPE)
        {
            std::string paramType = tokenStream.current().stringValue.getString();
            auto typeLocation = tokenStream.current().location;
            tokenStream.advance();

            // Check for nested generics
            if (tokenStream.check(TokenType::LESS))
            {
                paramType += "<";
                tokenStream.advance(); // consume '<'

                // Validate we have content after '<'
                if (tokenStream.check(TokenType::GREATER))
                {
                    throw ParseException(
                        "Malformed generic type '" + paramType + "<>': generic type arguments cannot be empty",
                        typeLocation);
                }

                paramType += parseGenericParameters(); // Recursive call

                // Validate matching '>'
                if (!tokenStream.check(TokenType::GREATER))
                {
                    throw ParseException(
                        "Malformed generic type '" + paramType + "': expected '>' to close generic type arguments",
                        typeLocation);
                }
                tokenStream.expect(TokenType::GREATER); // consume '>'
                paramType += ">";
            }

            return paramType;
        }

        throw ParseException("Expected type parameter", tokenStream.current().location);
    }

    std::vector<GenericTypeParameter> GenericParameterParser::parseGenericTypeParameters()
    {
        std::vector<GenericTypeParameter> parameters;

        // Parse first parameter
        parameters.push_back(parseGenericTypeParameter());

        // Parse additional parameters separated by commas
        while (tokenStream.check(TokenType::COMMA))
        {
            tokenStream.advance(); // consume ','
            parameters.push_back(parseGenericTypeParameter());
        }

        // Validate no circular dependencies in type bounds
        validateNoCircularDependencies(parameters);

        return parameters;
    }

    GenericTypeParameter GenericParameterParser::parseGenericTypeParameter()
    {
        // Expect an identifier for the type parameter name
        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected generic type parameter name", tokenStream.current().location);
        }

        std::string paramName = tokenStream.current().stringValue.getString();
        auto location = tokenStream.current().location;
        validateGenericParameterName(paramName);
        tokenStream.advance();

        // Parse optional constraints (extends/implements SomeInterface)
        std::vector<std::string> constraints;
        if (tokenStream.current().type == TokenType::EXTENDS ||
            tokenStream.current().type == TokenType::IMPLEMENTS)
        {
            tokenStream.advance(); // consume constraint keyword

            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected interface name after constraint keyword",
                                     tokenStream.current().location);
            }

            std::string constraintName = tokenStream.current().stringValue.getString();
            tokenStream.advance();

            // Handle generic parameters in constraints (e.g., Comparable<T>)
            if (tokenStream.current().type == TokenType::LESS)
            {
                constraintName += parseNestedGenericConstraint();
            }

            constraints.push_back(constraintName);

            // VALIDATION: Each generic parameter can only extend ONE interface
            // Check for additional extends/implements keywords
            if (tokenStream.current().type == TokenType::EXTENDS ||
                tokenStream.current().type == TokenType::IMPLEMENTS)
            {
                throw ParseException(
                    "Generic type parameter '" + paramName + "' can only extend one interface. "
                    "Multiple interface constraints are not supported.",
                    tokenStream.current().location);
            }
        }

        return GenericTypeParameter(paramName, constraints, location);
    }


    void GenericParameterParser::validateGenericParameterName(const std::string& paramName)
    {
        // Generic type parameters should typically be single uppercase letters (T, U, K, etc.)
        // But we'll be flexible and just ensure it's a valid identifier
        if (paramName.empty())
        {
            throw ParseException("Generic type parameter name cannot be empty", tokenStream.current().location);
        }
    }

    std::string GenericParameterParser::parseNestedGenericConstraint()
    {
        return ParserUtils::parseNestedGenericExpression(tokenStream);
    }

    void GenericParameterParser::validateNoCircularDependencies(const std::vector<GenericTypeParameter>& parameters)
    {
        // Check each parameter for circular dependencies
        for (const auto& param : parameters)
        {
            std::unordered_set<std::string> visited;
            std::unordered_set<std::string> recursionStack;

            if (hasCircularDependency(param.name, parameters, visited, recursionStack))
            {
                throw ParseException(
                    "Circular dependency detected in generic type parameter '" + param.name + "'. "
                    "Type parameters cannot have circular constraint relationships.",
                    param.location);
            }
        }
    }

    bool GenericParameterParser::hasCircularDependency(
        const std::string& paramName,
        const std::vector<GenericTypeParameter>& parameters,
        std::unordered_set<std::string>& visited,
        std::unordered_set<std::string>& recursionStack)
    {
        // If already visited in current DFS path (recursion stack), we have a cycle
        if (recursionStack.count(paramName))
        {
            return true;
        }

        // If already fully explored in previous DFS, no cycle from this node
        if (visited.count(paramName))
        {
            return false;
        }

        // Mark as visited and add to recursion stack
        visited.insert(paramName);
        recursionStack.insert(paramName);

        // Find the parameter definition
        const GenericTypeParameter* currentParam = nullptr;
        for (const auto& param : parameters)
        {
            if (param.name == paramName)
            {
                currentParam = &param;
                break;
            }
        }

        // If parameter not found, it's not a type parameter (could be a class name)
        if (!currentParam)
        {
            recursionStack.erase(paramName);
            return false;
        }

        // Check all constraints for this parameter
        for (const auto& constraint : currentParam->constraints)
        {
            // Extract type parameter names from constraint
            // e.g., "Box<U>" -> extract "U", "List<T, K>" -> extract "T" and "K"
            std::vector<std::string> referencedParams = extractTypeParameters(constraint, parameters);

            for (const auto& refParam : referencedParams)
            {
                // Allow self-references (e.g., T extends Comparable<T>)
                // This is a common and valid pattern, not a circular dependency
                if (refParam == paramName)
                {
                    continue;  // Skip self-references
                }

                if (hasCircularDependency(refParam, parameters, visited, recursionStack))
                {
                    return true;
                }
            }
        }

        // Remove from recursion stack (backtrack)
        recursionStack.erase(paramName);
        return false;
    }

    std::vector<std::string> GenericParameterParser::extractTypeParameters(
        const std::string& constraint,
        const std::vector<GenericTypeParameter>& parameters)
    {
        std::vector<std::string> result;

        // Build set of known type parameter names for quick lookup
        std::unordered_set<std::string> paramNames;
        for (const auto& param : parameters)
        {
            paramNames.insert(param.name);
        }

        // Extract identifiers from constraint string
        // e.g., "Box<U>" -> ["Box", "U"], "List<T, K>" -> ["List", "T", "K"]
        std::string currentToken;
        for (size_t i = 0; i < constraint.length(); ++i)
        {
            char c = constraint[i];

            if (std::isalnum(c) || c == '_')
            {
                currentToken += c;
            }
            else
            {
                if (!currentToken.empty())
                {
                    // Check if this token is a type parameter
                    if (paramNames.count(currentToken))
                    {
                        result.push_back(currentToken);
                    }
                    currentToken.clear();
                }
            }
        }

        // Don't forget the last token
        if (!currentToken.empty() && paramNames.count(currentToken))
        {
            result.push_back(currentToken);
        }

        return result;
    }
}
