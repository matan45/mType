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
		// Basic constructor for tokens without literal values
		Token(TokenType t, const std::string& l, int ln, int col = 0)
			: type(t), lexeme(l), literal(std::monostate{}), line(ln), column(col) {
		}

		// Static factory methods to avoid constructor ambiguity
		static Token withIntLiteral(TokenType t, const std::string& l, int value, int ln, int col = 0) {
			Token token(t, l, ln, col);
			token.literal = value;
			return token;
		}

		static Token withDoubleLiteral(TokenType t, const std::string& l, double value, int ln, int col = 0) {
			Token token(t, l, ln, col);
			token.literal = value;
			return token;
		}

		static Token withStringLiteral(TokenType t, const std::string& l, const std::string& value, int ln, int col = 0) {
			Token token(t, l, ln, col);
			token.literal = value;
			return token;
		}

		static Token withBoolLiteral(TokenType t, const std::string& l, bool value, int ln, int col = 0) {
			Token token(t, l, ln, col);
			token.literal = value;
			return token;
		}

		// Literal checking
		bool hasLiteral() const {
			return !std::holds_alternative<std::monostate>(literal);
		}

		// String representations
		std::string toString() const;
		static std::string typeToString(TokenType type);

		// Getters
		TokenType getType() const;
		const std::string& getLexeme() const;
		int getLine() const;
		int getColumn() const;

		// Generic literal getter
		template<typename T>
		bool getLiteral(T& out) const;

		// Specific literal getters
		bool getIntLiteral(int& value) const;
		bool getDoubleLiteral(double& value) const;
		bool getStringLiteral(std::string& value) const;
		bool getBoolLiteral(bool& value) const;

		// Type checking utilities
		bool is(TokenType t) const;
		bool isOneOf(std::initializer_list<TokenType> types) const;
		bool isLiteral() const;
		bool isKeyword() const;
		bool isOperator() const;
		bool isTypeKeyword() const;
		bool isAccessModifier() const;
	};

}
