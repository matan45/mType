#include "ParseContext.hpp"
#include "Parser.hpp"

namespace parser
{
    std::unique_ptr<ASTNode> ParseContext::parseStatement()
    {
        return parser->parseStatement();
    }

    std::unique_ptr<ASTNode> ParseContext::parseExpression()
    {
        return parser->parseExpression();
    }

    std::unique_ptr<ASTNode> ParseContext::parseClass()
    {
        return parser->parseClass();
    }

    std::unique_ptr<ASTNode> ParseContext::parseInterface()
    {
        return parser->parseInterface();
    }

    std::unique_ptr<ASTNode> ParseContext::parseAnnotationDeclaration()
    {
        return parser->parseAnnotationDeclaration();
    }

    std::unique_ptr<ASTNode> ParseContext::parseNewExpression()
    {
        return parser->parseNewExpression();
    }
}
