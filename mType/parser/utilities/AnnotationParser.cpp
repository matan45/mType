#include "AnnotationParser.hpp"
#include "../../errors/ParseException.hpp"
namespace parser::utilities
{
    std::shared_ptr<AnnotationNode> AnnotationParser::parseAnnotation(TokenStream& tokenStream)
    {
        // Check if current token is '@'
        if (!isAnnotation(tokenStream.current().type))
        {
            return nullptr;
        }

        // Consume '@' token
        auto atLocation = tokenStream.current().location;
        tokenStream.advance();

        // Expect identifier (annotation name)
        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException(
                "Expected annotation name after '@'",
                tokenStream.current().location
            );
        }

        std::string annotationName = tokenStream.current().stringValue.getString();
        auto annotationLocation = tokenStream.current().location;
        tokenStream.advance();

        // Parse annotation parameters if present (e.g., @Throw(IOException, NetworkException))
        std::unordered_map<std::string, std::string> parameters;
        if (tokenStream.current().type == TokenType::LPAREN)
        {
            parameters = parseAnnotationParameters(tokenStream);
        }

        return std::make_shared<AnnotationNode>(
            annotationName,
            parameters,
            annotationLocation
        );
    }

    std::vector<std::shared_ptr<AnnotationNode>> AnnotationParser::parseAnnotations(TokenStream& tokenStream)
    {
        std::vector<std::shared_ptr<AnnotationNode>> annotations;

        // Parse all consecutive annotations
        // Invariant: parseAnnotation() never returns nullptr when isAnnotation() returns true
        // It either returns a valid shared_ptr or throws ParseException
        while (isAnnotation(tokenStream.current().type))
        {
            auto annotation = parseAnnotation(tokenStream);

            // This should never be null due to the loop condition
            // If it is null, there's a critical bug in parseAnnotation()
            if (!annotation)
            {
                throw ParseException(
                    "Internal error: parseAnnotation returned null after isAnnotation check passed",
                    tokenStream.current().location
                );
            }

            annotations.push_back(annotation);
        }

        return annotations;
    }

    bool AnnotationParser::isAnnotation(TokenType type)
    {
        return type == TokenType::AT;
    }

    std::unordered_map<std::string, std::string> AnnotationParser::parseAnnotationParameters(TokenStream& tokenStream)
    {
        std::unordered_map<std::string, std::string> parameters;
        std::vector<std::string> exceptionNames;

        // Consume LPAREN
        auto leftParenLocation = tokenStream.current().location;
        if (tokenStream.current().type != TokenType::LPAREN)
        {
            throw ParseException(
                "Expected '(' after annotation name",
                tokenStream.current().location
            );
        }
        tokenStream.advance();

        // Parse comma-separated list of exception class names
        while (tokenStream.current().type != TokenType::RPAREN)
        {
            // Expect IDENTIFIER (exception class name)
            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException(
                    "Expected exception class name in annotation parameters",
                    tokenStream.current().location
                );
            }

            std::string exceptionName = tokenStream.current().stringValue.getString();
            exceptionNames.push_back(exceptionName);
            tokenStream.advance();

            // Check for comma or closing paren
            if (tokenStream.current().type == TokenType::COMMA)
            {
                tokenStream.advance();
                // After comma, there must be another identifier
                if (tokenStream.current().type == TokenType::RPAREN)
                {
                    throw ParseException(
                        "Expected exception class name after ',' in annotation parameters",
                        tokenStream.current().location
                    );
                }
            }
            else if (tokenStream.current().type != TokenType::RPAREN)
            {
                throw ParseException(
                    "Expected ',' or ')' after exception class name in annotation parameters",
                    tokenStream.current().location
                );
            }
        }

        // Validate that at least one exception name was provided
        if (exceptionNames.empty())
        {
            throw ParseException(
                "Empty annotation parameters not allowed. Expected at least one exception class name",
                leftParenLocation
            );
        }

        // Consume RPAREN
        tokenStream.advance();

        // Join exception names with comma separator and store in parameters map
        std::string exceptionsParam;
        for (size_t i = 0; i < exceptionNames.size(); ++i)
        {
            if (i > 0)
            {
                exceptionsParam += ",";
            }
            exceptionsParam += exceptionNames[i];
        }
        parameters["exceptions"] = exceptionsParam;

        return parameters;
    }
}
