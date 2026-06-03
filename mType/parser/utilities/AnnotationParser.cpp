#include "AnnotationParser.hpp"
#include <cstdint>
#include "../../errors/ParseException.hpp"
#include "../ParseContext.hpp"
#include "../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../ast/nodes/expressions/FloatNode.hpp"
#include "../../ast/nodes/expressions/BoolNode.hpp"
#include "../../ast/nodes/expressions/StringNode.hpp"
#include "../../ast/nodes/expressions/ArrayLiteralNode.hpp"

namespace parser::utilities
{
    namespace
    {
        // True when `t` is a binary/ternary operator that would continue an
        // expression after a leading literal token — used to decide whether an
        // annotation argument that starts with a literal is actually a constant
        // expression (e.g. `60 * 1000`) rather than a plain literal.
        bool isBinaryOpContinuation(TokenType t)
        {
            switch (t)
            {
            case TokenType::PLUS:
            case TokenType::MINUS:
            case TokenType::MULTIPLY:
            case TokenType::DIVIDE:
            case TokenType::MODULO:
            case TokenType::AND:
            case TokenType::OR:
            case TokenType::BITWISE_AND:
            case TokenType::BITWISE_OR:
            case TokenType::BITWISE_XOR:
            case TokenType::LEFT_SHIFT:
            case TokenType::RIGHT_SHIFT:
            case TokenType::EQUALS:
            case TokenType::NOT_EQUALS:
            case TokenType::LESS:
            case TokenType::LESS_EQUALS:
            case TokenType::GREATER:
            case TokenType::GREATER_EQUALS:
            case TokenType::QUESTION:
                return true;
            default:
                return false;
            }
        }

        // Synthesize a literal AST node from a plain annotation value so it can
        // be embedded as an element of a deferred ArrayLiteralNode. Returns
        // null for non-foldable kinds (class refs / null / nested arrays),
        // which the caller rejects.
        std::unique_ptr<ast::ASTNode> typedValueToLiteralNode(const TypedAnnotationValue& v,
                                                              const SourceLocation& loc)
        {
            switch (v.getType())
            {
            case AnnotationValueType::INT:
                return std::make_unique<ast::nodes::expressions::IntegerNode>(v.asInt(), loc);
            case AnnotationValueType::FLOAT:
                return std::make_unique<ast::nodes::expressions::FloatNode>(v.asFloat(), loc);
            case AnnotationValueType::BOOL:
                return std::make_unique<ast::nodes::expressions::BoolNode>(v.asBool(), loc);
            case AnnotationValueType::STRING:
                return std::make_unique<ast::nodes::expressions::StringNode>(v.asString(), loc);
            default:
                return nullptr;
            }
        }
    }

    std::shared_ptr<AnnotationNode> AnnotationParser::parseAnnotation(TokenStream& tokenStream,
                                                                     ParseContext* context)
    {
        if (!isAnnotation(tokenStream.current().type))
        {
            return nullptr;
        }

        tokenStream.advance(); // consume '@'

        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected annotation name after '@'", tokenStream.current().location);
        }

        std::string annotationName = std::string(tokenStream.current().stringValue);
        SourceLocation annotationLocation = tokenStream.current().location;
        tokenStream.advance();

        auto node = std::make_shared<AnnotationNode>(annotationName, annotationLocation);

        if (tokenStream.current().type == TokenType::LPAREN)
        {
            parseAnnotationArguments(tokenStream, *node, context);
        }

        return node;
    }

    std::vector<std::shared_ptr<AnnotationNode>> AnnotationParser::parseAnnotations(TokenStream& tokenStream,
                                                                                   ParseContext* context)
    {
        std::vector<std::shared_ptr<AnnotationNode>> annotations;
        while (isAnnotation(tokenStream.current().type))
        {
            auto annotation = parseAnnotation(tokenStream, context);
            if (!annotation)
            {
                throw ParseException("Internal error: parseAnnotation returned null after isAnnotation passed",
                                     tokenStream.current().location);
            }
            annotations.push_back(annotation);
        }
        return annotations;
    }

    bool AnnotationParser::isAnnotation(TokenType type)
    {
        return type == TokenType::AT;
    }

    void AnnotationParser::parseAnnotationArguments(TokenStream& tokenStream, AnnotationNode& target,
                                                    ParseContext* context)
    {
        SourceLocation lparenLoc = tokenStream.current().location;
        tokenStream.advance(); // consume '('

        // Empty `()` is allowed — the validator enforces required-parameter checks
        // against the AnnotationDefinition.
        if (tokenStream.current().type == TokenType::RPAREN)
        {
            tokenStream.advance();
            return;
        }

        // Disambiguate:
        //   IDENT = value              -> named argument
        //   IDENT , IDENT [, ...]      -> legacy bare-identifier list (@Throw(IO,Net))
        //   anything else (incl. lone  -> positional shorthand (single value)
        //   IDENT before RPAREN)
        //
        // The legacy form requires AT LEAST two identifiers separated by a
        // comma — a lone `IDENT RPAREN` would otherwise collide with the
        // single-arg positional form (`@Retention(SOURCE)`, `@Throw(IOExc)`),
        // which is a more natural spelling for one-shot Class-ref
        // annotations. A single bare IDENT still reaches @Throw via the
        // CLASS_REF → CLASS_ARRAY widening in the usage validator.
        bool firstIsIdent  = (tokenStream.current().type == TokenType::IDENTIFIER);
        bool secondIsAssign =
            firstIsIdent && (tokenStream.peek().type == TokenType::ASSIGN);
        bool legacyIdentList =
            firstIsIdent && !secondIsAssign &&
            tokenStream.peek().type == TokenType::COMMA;

        if (legacyIdentList)
        {
            // Collect bare identifiers as a Class[]-style ordered list and
            // store under the legacy `"exceptions"` key so unmigrated readers
            // (currently @Throw validator) keep functioning.
            std::vector<std::string> classNames;
            while (tokenStream.current().type != TokenType::RPAREN)
            {
                if (tokenStream.current().type != TokenType::IDENTIFIER)
                {
                    throw ParseException("Expected class identifier in annotation argument list",
                                         tokenStream.current().location);
                }
                classNames.push_back(std::string(tokenStream.current().stringValue));
                tokenStream.advance();
                if (tokenStream.current().type == TokenType::COMMA)
                {
                    tokenStream.advance();
                    if (tokenStream.current().type == TokenType::RPAREN)
                    {
                        throw ParseException("Trailing ',' in annotation argument list",
                                             tokenStream.current().location);
                    }
                }
                else if (tokenStream.current().type != TokenType::RPAREN)
                {
                    throw ParseException("Expected ',' or ')' in annotation argument list",
                                         tokenStream.current().location);
                }
            }
            tokenStream.advance(); // consume ')'

            if (classNames.empty())
            {
                throw ParseException("Empty annotation parameters not allowed in legacy form",
                                     lparenLoc);
            }
            // Legacy compatibility key — also exposed to the new validator via
            // typed value of CLASS_ARRAY type.
            target.setTypedParameter("exceptions",
                TypedAnnotationValue::makeClassArray(std::move(classNames)));
            return;
        }

        if (secondIsAssign)
        {
            // Named-argument form: key = value, key = value, ...
            while (tokenStream.current().type != TokenType::RPAREN)
            {
                if (tokenStream.current().type != TokenType::IDENTIFIER)
                {
                    throw ParseException("Expected annotation parameter name",
                                         tokenStream.current().location);
                }
                std::string key = std::string(tokenStream.current().stringValue);
                SourceLocation keyLoc = tokenStream.current().location;
                tokenStream.advance();

                if (tokenStream.current().type != TokenType::ASSIGN)
                {
                    throw ParseException("Expected '=' after annotation parameter name",
                                         tokenStream.current().location);
                }
                tokenStream.advance();

                if (target.hasParameter(key) || target.getDeferredExpressions().count(key))
                {
                    throw ParseException("Duplicate annotation parameter '" + key + "'", keyLoc);
                }
                ParsedValue pv = parseAnnotationValue(tokenStream, context);
                if (pv.isDeferred())
                {
                    target.setDeferredExpression(key, std::shared_ptr<ast::ASTNode>(std::move(pv.deferred)));
                }
                else
                {
                    target.setTypedParameter(key, std::move(*pv.value));
                }

                if (tokenStream.current().type == TokenType::COMMA)
                {
                    tokenStream.advance();
                    if (tokenStream.current().type == TokenType::RPAREN)
                    {
                        throw ParseException("Trailing ',' in annotation argument list",
                                             tokenStream.current().location);
                    }
                }
                else if (tokenStream.current().type != TokenType::RPAREN)
                {
                    throw ParseException("Expected ',' or ')' after annotation argument",
                                         tokenStream.current().location);
                }
            }
            tokenStream.advance(); // consume ')'
            return;
        }

        // Positional shorthand: single value. Semantic binding to the sole
        // declared parameter happens in the usage validator (M4) — here we
        // just stash it under the reserved key "__positional__".
        ParsedValue pv = parseAnnotationValue(tokenStream, context);
        if (pv.isDeferred())
        {
            target.setDeferredExpression("__positional__", std::shared_ptr<ast::ASTNode>(std::move(pv.deferred)));
        }
        else
        {
            target.setTypedParameter("__positional__", std::move(*pv.value));
        }

        if (tokenStream.current().type != TokenType::RPAREN)
        {
            throw ParseException("Expected ')' after positional annotation argument",
                                 tokenStream.current().location);
        }
        tokenStream.advance(); // consume ')'
    }

    AnnotationParser::ParsedValue AnnotationParser::parseAnnotationValue(TokenStream& tokenStream,
                                                                        ParseContext* context)
    {
        const Token& tok = tokenStream.current();
        const SourceLocation valueLoc = tok.location; // copy — survives stream advance

        // Helper: parse a constant expression (deferred until the resolver
        // pass). Requires a ParseContext to drive the expression parser.
        auto parseExpr = [&]() -> ParsedValue
        {
            if (!context)
            {
                throw ParseException(
                    "Expression-valued annotation arguments are not supported in this position",
                    valueLoc);
            }
            auto expr = context->parseExpression();
            if (!expr)
            {
                throw ParseException("Failed to parse annotation argument expression", valueLoc);
            }
            return ParsedValue::ofDeferred(std::move(expr));
        };

        switch (tok.type)
        {
        case TokenType::NULL_LITERAL:
            tokenStream.advance();
            return ParsedValue::ofValue(TypedAnnotationValue::makeNull());
        case TokenType::INT_NUMBER:
        {
            if (isBinaryOpContinuation(tokenStream.peek().type)) return parseExpr();
            int64_t v = tok.intValue;
            tokenStream.advance();
            return ParsedValue::ofValue(TypedAnnotationValue::makeInt(v));
        }
        case TokenType::FLOAT_NUMBER:
        {
            if (isBinaryOpContinuation(tokenStream.peek().type)) return parseExpr();
            double v = tok.floatValue;
            tokenStream.advance();
            return ParsedValue::ofValue(TypedAnnotationValue::makeFloat(v));
        }
        case TokenType::TRUE:
            if (isBinaryOpContinuation(tokenStream.peek().type)) return parseExpr();
            tokenStream.advance();
            return ParsedValue::ofValue(TypedAnnotationValue::makeBool(true));
        case TokenType::FALSE:
            if (isBinaryOpContinuation(tokenStream.peek().type)) return parseExpr();
            tokenStream.advance();
            return ParsedValue::ofValue(TypedAnnotationValue::makeBool(false));
        case TokenType::STRING_LITERAL:
        {
            if (isBinaryOpContinuation(tokenStream.peek().type)) return parseExpr();
            std::string v = std::string(tok.stringValue);
            tokenStream.advance();
            return ParsedValue::ofValue(TypedAnnotationValue::makeString(std::move(v)));
        }
        case TokenType::IDENTIFIER:
        {
            // `Class::FIELD` constant reference, a call, or member access →
            // expression mode (folded / rejected by the resolver). A bare
            // identifier stays a CLASS_REF (preserves @Throw / @Target).
            const TokenType pk = tokenStream.peek().type;
            if (pk == TokenType::SCOPE || pk == TokenType::LPAREN ||
                pk == TokenType::DOT || pk == TokenType::QUESTION_DOT)
            {
                return parseExpr();
            }
            std::string v = std::string(tok.stringValue);
            tokenStream.advance();
            return ParsedValue::ofValue(TypedAnnotationValue::makeClassRef(std::move(v)));
        }
        case TokenType::LBRACKET:
            return parseArrayLiteral(tokenStream, context);
        // Leading unary / parenthesized / cast / construction → expression.
        case TokenType::MINUS:
        case TokenType::NOT:
        case TokenType::BITWISE_NOT:
        case TokenType::LPAREN:
        case TokenType::NEW:
            return parseExpr();
        default:
            throw ParseException(
                "Expected annotation literal value (int, float, bool, string, identifier, null, or supported array literal)",
                tok.location);
        }
    }

    AnnotationParser::ParsedValue AnnotationParser::parseArrayLiteral(TokenStream& tokenStream,
                                                                     ParseContext* context)
    {
        SourceLocation lbracketLoc = tokenStream.current().location;
        tokenStream.advance(); // consume '['

        if (tokenStream.current().type == TokenType::RBRACKET)
        {
            tokenStream.advance();
            return ParsedValue::ofValue(TypedAnnotationValue::makeClassArray({}));
        }

        std::vector<ParsedValue> elements;
        bool anyDeferred = false;
        while (tokenStream.current().type != TokenType::RBRACKET)
        {
            ParsedValue elem = parseAnnotationValue(tokenStream, context);
            if (elem.isDeferred()) anyDeferred = true;
            elements.push_back(std::move(elem));

            if (tokenStream.current().type == TokenType::COMMA)
            {
                tokenStream.advance();
                if (tokenStream.current().type == TokenType::RBRACKET)
                {
                    throw ParseException("Trailing ',' in annotation array literal",
                                         tokenStream.current().location);
                }
            }
            else if (tokenStream.current().type != TokenType::RBRACKET)
            {
                throw ParseException("Expected ',' or ']' in annotation array literal",
                                     tokenStream.current().location);
            }
        }
        tokenStream.advance(); // consume ']'

        if (anyDeferred)
        {
            // Defer the whole array as an ArrayLiteralNode; AnnotationConstantResolver
            // folds each element and re-derives the homogeneous array kind.
            std::vector<std::unique_ptr<ast::ASTNode>> nodes;
            nodes.reserve(elements.size());
            for (auto& e : elements)
            {
                if (e.isDeferred())
                {
                    nodes.push_back(std::move(e.deferred));
                }
                else
                {
                    auto lit = typedValueToLiteralNode(*e.value, lbracketLoc);
                    if (!lit)
                    {
                        throw ParseException(
                            "Annotation array element type mismatch: cannot mix class references "
                            "with constant expressions",
                            lbracketLoc);
                    }
                    nodes.push_back(std::move(lit));
                }
            }
            auto arr = std::make_unique<ast::nodes::expressions::ArrayLiteralNode>(
                std::move(nodes), lbracketLoc);
            return ParsedValue::ofDeferred(std::move(arr));
        }

        return ParsedValue::ofValue(buildLiteralArray(elements, lbracketLoc));
    }

    TypedAnnotationValue AnnotationParser::buildLiteralArray(std::vector<ParsedValue>& elements,
                                                            const SourceLocation& loc)
    {
        const AnnotationValueType first = elements.front().value->getType();

        if (first == AnnotationValueType::CLASS_REF)
        {
            std::vector<std::string> entries;
            for (auto& e : elements)
            {
                if (e.value->getType() != AnnotationValueType::CLASS_REF)
                {
                    throw ParseException("Expected class identifier inside annotation Class[] literal", loc);
                }
                entries.push_back(e.value->asClassRef());
            }
            return TypedAnnotationValue::makeClassArray(std::move(entries));
        }

        if (first == AnnotationValueType::STRING)
        {
            std::vector<std::string> entries;
            for (auto& e : elements)
            {
                if (e.value->getType() != AnnotationValueType::STRING)
                {
                    throw ParseException("Expected string literal inside annotation string[] literal", loc);
                }
                entries.push_back(e.value->asString());
            }
            return TypedAnnotationValue::makeStringArray(std::move(entries));
        }

        if (first == AnnotationValueType::BOOL)
        {
            std::vector<bool> entries;
            for (auto& e : elements)
            {
                if (e.value->getType() != AnnotationValueType::BOOL)
                {
                    throw ParseException("Expected boolean literal inside annotation bool[] literal", loc);
                }
                entries.push_back(e.value->asBool());
            }
            return TypedAnnotationValue::makeBoolArray(std::move(entries));
        }

        if (first == AnnotationValueType::INT || first == AnnotationValueType::FLOAT)
        {
            std::vector<int64_t> ints;
            std::vector<double> floats;
            bool hasFloat = false;
            for (auto& e : elements)
            {
                const AnnotationValueType t = e.value->getType();
                if (t == AnnotationValueType::INT)
                {
                    ints.push_back(e.value->asInt());
                    floats.push_back(static_cast<double>(e.value->asInt()));
                }
                else if (t == AnnotationValueType::FLOAT)
                {
                    hasFloat = true;
                    floats.push_back(e.value->asFloat());
                }
                else
                {
                    throw ParseException("Expected numeric literal inside annotation numeric[] literal", loc);
                }
            }
            if (hasFloat) return TypedAnnotationValue::makeFloatArray(std::move(floats));
            return TypedAnnotationValue::makeIntArray(std::move(ints));
        }

        throw ParseException("Unsupported annotation array literal element type", loc);
    }
}
