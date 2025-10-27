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

        std::string annotationName = tokenStream.current().stringValue;
        auto annotationLocation = tokenStream.current().location;
        tokenStream.advance();

        // TODO: In future, support annotation parameters like @Annotation("value")
        // For now, only support simple @AnnotationName syntax
        std::unordered_map<std::string, std::string> parameters;

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
        while (isAnnotation(tokenStream.current().type))
        {
            auto annotation = parseAnnotation(tokenStream);
            if (annotation)
            {
                annotations.push_back(annotation);
            }
            else
            {
                break;
            }
        }

        return annotations;
    }

    bool AnnotationParser::isAnnotation(TokenType type)
    {
        return type == TokenType::AT;
    }
}
