#pragma once

#include <string>
#include <variant>

namespace mtype {
	namespace core {
		enum class TokenType {
			// Literals
			NUMBER, STRING, IDENTIFIER, TRUE, FALSE, NULL_TOKEN,

			// Keywords
			IF, ELSE, WHILE, FOR, FUNCTION, RETURN, CLASS, NEW, THIS, IMPORT,
			INT, FLOAT, BOOL, STR, VOID, EXTENDS, SUPER, CONSTRUCTOR,
			EXPORT, MODULE, FROM, STATIC, NAMESPACE, USING,
			PUBLIC, PRIVATE, ENUM, ARRAY,

			// Operators
			PLUS, MINUS, STAR, SLASH, PERCENT,
			EQUAL, EQUAL_EQUAL, NOT_EQUAL,
			LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
			AND, OR, NOT,

			// Delimiters
			LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
			SEMICOLON, COMMA, DOT, COLON, DOUBLE_COLON,

			// Special
			END_OF_FILE, NEWLINE
		};


		struct Token {
			TokenType type;
			std::string lexeme;
			std::variant<int, double, std::string, bool, std::monostate> literal;
			int line;
			int column;

			Token(TokenType t, const std::string& l, int ln, int col = 0)
				: type(t), lexeme(l), literal(std::monostate{}), line(ln), column(col) {
			}

			Token(TokenType t, const std::string& l, int value, int ln, int col = 0)
				: type(t), lexeme(l), literal(value), line(ln), column(col) {
			}

			Token(TokenType t, const std::string& l, double value, int ln, int col = 0)
				: type(t), lexeme(l), literal(value), line(ln), column(col) {
			}

			Token(TokenType t, const std::string& l, const std::string& value, int ln, int col = 0)
				: type(t), lexeme(l), literal(value), line(ln), column(col) {
			}

			Token(TokenType t, const std::string& l, bool value, int ln, int col = 0)
				: type(t), lexeme(l), literal(value), line(ln), column(col) {
			}

			bool hasLiteral() const {
				return !std::holds_alternative<std::monostate>(literal);
			}

			std::string toString() const;
			static std::string typeToString(TokenType type);
		};
	}
}
