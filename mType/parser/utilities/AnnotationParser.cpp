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
}
