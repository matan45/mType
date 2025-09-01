#include "ParseContext.hpp"
#include "StatementParser.hpp"
#include "ExpressionParser.hpp"
#include "ClassParser.hpp"
#include "TokenStream.hpp"
#include "../errors/ParseException.hpp"

namespace parser
{
    using namespace errors;

    ParseContext::ParseContext(StatementParser* stmt, ExpressionParser* expr, ClassParser* cls, TokenStream* stream)
        : statementParser(stmt), expressionParser(expr), classParser(cls), tokenStream(stream)
    {
    }

    std::unique_ptr<ASTNode> ParseContext::parseStatement()
    {
        if (!statementParser)
        {
            auto location = tokenStream ? tokenStream->location() : errors::SourceLocation{};
            throw ParseException("StatementParser not initialized", location);
        }
        return statementParser->parseStatement();
    }

    std::unique_ptr<ASTNode> ParseContext::parseExpression()
    {
        if (!expressionParser)
        {
            auto location = tokenStream ? tokenStream->location() : errors::SourceLocation{};
            throw ParseException("ExpressionParser not initialized", location);
        }
        return expressionParser->parseExpression();
    }

    std::unique_ptr<ASTNode> ParseContext::parseClass()
    {
        if (!classParser)
        {
            auto location = tokenStream ? tokenStream->location() : errors::SourceLocation{};
            throw ParseException("ClassParser not initialized", location);
        }
        return classParser->parseClass();
    }

    std::unique_ptr<ASTNode> ParseContext::parseNewExpression()
    {
        if (!classParser)
        {
            auto location = tokenStream ? tokenStream->location() : errors::SourceLocation{};
            throw ParseException("ClassParser not initialized", location);
        }
        return classParser->parseNewExpression();
    }
}