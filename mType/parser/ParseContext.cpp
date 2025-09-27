#include "ParseContext.hpp"
#include "StatementParser.hpp"
#include "ExpressionParser.hpp"
#include "ClassParser.hpp"
#include "InterfaceParser.hpp"
#include "TokenStream.hpp"
#include "../errors/ParseException.hpp"

namespace parser
{
    using namespace errors;

    ParseContext::ParseContext(StatementParser& stmt, ExpressionParser& expr, ClassParser& cls, InterfaceParser& iface, TokenStream& stream)
        : statementParser(std::ref(stmt)),
          expressionParser(std::ref(expr)),
          classParser(std::ref(cls)),
          interfaceParser(std::ref(iface)),
          tokenStream(std::ref(stream))
    {
    }

    std::unique_ptr<ASTNode> ParseContext::parseStatement()
    {
        if (!statementParser.has_value())
        {
            auto location = tokenStream.has_value() ? tokenStream->get().location() : errors::SourceLocation{};
            throw ParseException("StatementParser not initialized", location);
        }
        return statementParser->get().parseStatement();
    }

    std::unique_ptr<ASTNode> ParseContext::parseExpression()
    {
        if (!expressionParser.has_value())
        {
            auto location = tokenStream.has_value() ? tokenStream->get().location() : errors::SourceLocation{};
            throw ParseException("ExpressionParser not initialized", location);
        }
        return expressionParser->get().parseExpression();
    }

    std::unique_ptr<ASTNode> ParseContext::parseClass()
    {
        if (!classParser.has_value())
        {
            auto location = tokenStream.has_value() ? tokenStream->get().location() : errors::SourceLocation{};
            throw ParseException("ClassParser not initialized", location);
        }
        return classParser->get().parseClass();
    }

    std::unique_ptr<ASTNode> ParseContext::parseInterface()
    {
        if (!interfaceParser.has_value())
        {
            auto location = tokenStream.has_value() ? tokenStream->get().location() : errors::SourceLocation{};
            throw ParseException("InterfaceParser not initialized", location);
        }
        return interfaceParser->get().parseInterface();
    }

    std::unique_ptr<ASTNode> ParseContext::parseNewExpression()
    {
        if (!classParser.has_value())
        {
            auto location = tokenStream.has_value() ? tokenStream->get().location() : errors::SourceLocation{};
            throw ParseException("ClassParser not initialized", location);
        }
        return classParser->get().parseNewExpression();
    }
}