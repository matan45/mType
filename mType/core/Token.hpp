#pragma once

#include <string>
#include <variant>

namespace mtype::core {

	enum class TokenType {
		// Literals
		NUMBER, STRING_LITERALS, IDENTIFIER, TRUE, FALSE, NULL_TOKEN,

		// Keywords
		IF, ELSE, SWITCH, CASE, DEFAULT, WHILE, FOR, DO, BREAK, CONTINUE,
		FUNCTION, RETURN, CLASS, NEW, THIS, ENUM,
		INT, FLOAT, BOOL, STRING, VOID, EXTENDS, SUPER, CONSTRUCTOR,
		MODULE, STATIC, NAMESPACE, USING,
		PUBLIC, PRIVATE, FINAL, 

		// Operators
		PLUS, MINUS, STAR, SLASH, PERCENT,
		EQUAL, EQUAL_EQUAL, NOT_EQUAL,
		LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
		AND, OR, NOT, ASSIGN,
		PLUS_PLUS, MINUS_MINUS, PLUS_EQUAL, MINUS_EQUAL,

		// Delimiters
		LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
		SEMICOLON, COMMA, DOT, COLON, DOUBLE_COLON,

		// Special
		END_OF_FILE, NEWLINE
	};

	// DOUBLE_COLON and USING for namespace resolution
	//TODO: need to add double forward slash and /* for comments


	class Token {
	private:
		TokenType type;
		std::string lexeme;
		std::variant<int, double, std::string, bool, std::monostate> literal;
		int line;
		int column;
	public:
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
