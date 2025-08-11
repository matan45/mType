#include "Token.hpp"
#include <sstream>

namespace mtype::core {
	std::string Token::toString() const {
		std::ostringstream oss;
		oss << "Token{" << typeToString(type) << ", '" << lexeme << "'";

		// Add literal value if present
		if (hasLiteral()) {
			oss << ", ";
			std::visit([&oss](const auto& value) {
				using T = std::decay_t<decltype(value)>;
				if constexpr (std::is_same_v<T, int>) {
					oss << value;
				}
				else if constexpr (std::is_same_v<T, double>) {
					oss << value;
				}
				else if constexpr (std::is_same_v<T, std::string>) {
					oss << "\"" << value << "\"";
				}
				else if constexpr (std::is_same_v<T, bool>) {
					oss << (value ? "true" : "false");
				}
				// std::monostate case is handled by hasLiteral() check
				}, literal);
		}

		oss << ", " << line << ":" << column << "}";
		return oss.str();
	}

	std::string Token::typeToString(TokenType type) {
		switch (type) {
			// Literals
		case TokenType::NUMBER: return "NUMBER";
		case TokenType::STRING_LITERALS: return "STRING_LITERALS";
		case TokenType::IDENTIFIER: return "IDENTIFIER";
		case TokenType::TRUE: return "TRUE";
		case TokenType::FALSE: return "FALSE";
		case TokenType::NULL_TOKEN: return "NULL_TOKEN";

			// Keywords
		case TokenType::IF: return "IF";
		case TokenType::ELSE: return "ELSE";
		case TokenType::SWITCH: return "SWITCH";
		case TokenType::CASE: return "CASE";
		case TokenType::DEFAULT: return "DEFAULT";
		case TokenType::WHILE: return "WHILE";
		case TokenType::FOR: return "FOR";
		case TokenType::DO: return "DO";
		case TokenType::BREAK: return "BREAK";
		case TokenType::CONTINUE: return "CONTINUE";
		case TokenType::FUNCTION: return "FUNCTION";
		case TokenType::RETURN: return "RETURN";
		case TokenType::CLASS: return "CLASS";
		case TokenType::NEW: return "NEW";
		case TokenType::THIS: return "THIS";
		case TokenType::ENUM: return "ENUM";
		case TokenType::INT: return "INT";
		case TokenType::FLOAT: return "FLOAT";
		case TokenType::BOOL: return "BOOL";
		case TokenType::STRING: return "STRING";
		case TokenType::VOID: return "VOID";
		case TokenType::EXTENDS: return "EXTENDS";
		case TokenType::SUPER: return "SUPER";
		case TokenType::CONSTRUCTOR: return "CONSTRUCTOR";
		case TokenType::IMPORT: return "IMPORT";
		case TokenType::STATIC: return "STATIC";
		case TokenType::NAMESPACE: return "NAMESPACE";
		case TokenType::USING: return "USING";
		case TokenType::PUBLIC: return "PUBLIC";
		case TokenType::PRIVATE: return "PRIVATE";
		case TokenType::FINAL: return "FINAL";

			// Operators
		case TokenType::PLUS: return "PLUS";
		case TokenType::MINUS: return "MINUS";
		case TokenType::STAR: return "STAR";
		case TokenType::SLASH: return "SLASH";
		case TokenType::PERCENT: return "PERCENT";
		case TokenType::EQUAL: return "EQUAL";
		case TokenType::EQUAL_EQUAL: return "EQUAL_EQUAL";
		case TokenType::NOT_EQUAL: return "NOT_EQUAL";
		case TokenType::LESS: return "LESS";
		case TokenType::LESS_EQUAL: return "LESS_EQUAL";
		case TokenType::GREATER: return "GREATER";
		case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
		case TokenType::AND: return "AND";
		case TokenType::OR: return "OR";
		case TokenType::NOT: return "NOT";
		case TokenType::ASSIGN: return "ASSIGN";
		case TokenType::PLUS_PLUS: return "PLUS_PLUS";
		case TokenType::MINUS_MINUS: return "MINUS_MINUS";
		case TokenType::PLUS_EQUAL: return "PLUS_EQUAL";
		case TokenType::MINUS_EQUAL: return "MINUS_EQUAL";

			// Delimiters
		case TokenType::LPAREN: return "LPAREN";
		case TokenType::RPAREN: return "RPAREN";
		case TokenType::LBRACE: return "LBRACE";
		case TokenType::RBRACE: return "RBRACE";
		case TokenType::LBRACKET: return "LBRACKET";
		case TokenType::RBRACKET: return "RBRACKET";
		case TokenType::SEMICOLON: return "SEMICOLON";
		case TokenType::COMMA: return "COMMA";
		case TokenType::DOT: return "DOT";
		case TokenType::COLON: return "COLON";
		case TokenType::DOUBLE_COLON: return "DOUBLE_COLON";

			// Special
		case TokenType::END_OF_FILE: return "END_OF_FILE";
		case TokenType::NEWLINE: return "NEWLINE";

		default: return "UNKNOWN";
		}
	}

	// Getter methods for accessing token properties
	TokenType Token::getType() const {
		return type;
	}

	const std::string& Token::getLexeme() const {
		return lexeme;
	}

	int Token::getLine() const {
		return line;
	}

	int Token::getColumn() const {
		return column;
	}

	// Template method to get literal value safely
	template<typename T>
	bool Token::getLiteral(T& out) const {
		if (std::holds_alternative<T>(literal)) {
			out = std::get<T>(literal);
			return true;
		}
		return false;
	}

	// Convenience methods for getting specific literal types
	bool Token::getIntLiteral(int& value) const {
		return getLiteral<int>(value);
	}

	bool Token::getDoubleLiteral(double& value) const {
		return getLiteral<double>(value);
	}

	bool Token::getStringLiteral(std::string& value) const {
		return getLiteral<std::string>(value);
	}

	bool Token::getBoolLiteral(bool& value) const {
		return getLiteral<bool>(value);
	}

	// Check if token matches a specific type
	bool Token::is(TokenType t) const {
		return type == t;
	}

	bool Token::isOneOf(std::initializer_list<TokenType> types) const {
		for (TokenType t : types) {
			if (type == t) return true;
		}
		return false;
	}

	// Check if token is a literal
	bool Token::isLiteral() const {
		return isOneOf({ TokenType::NUMBER, TokenType::STRING_LITERALS,
						TokenType::TRUE, TokenType::FALSE, TokenType::NULL_TOKEN });
	}

	// Check if token is a keyword
	bool Token::isKeyword() const {
		return isOneOf({
			TokenType::IF, TokenType::ELSE, TokenType::SWITCH, TokenType::CASE, TokenType::DEFAULT,
			TokenType::WHILE, TokenType::FOR, TokenType::DO, TokenType::BREAK, TokenType::CONTINUE,
			TokenType::FUNCTION, TokenType::RETURN, TokenType::CLASS, TokenType::NEW, TokenType::THIS,
			TokenType::ENUM, TokenType::INT, TokenType::FLOAT, TokenType::BOOL, TokenType::STRING,
			TokenType::VOID, TokenType::EXTENDS, TokenType::SUPER, TokenType::CONSTRUCTOR,
			TokenType::IMPORT, TokenType::STATIC, TokenType::NAMESPACE, TokenType::USING,
			TokenType::PUBLIC, TokenType::PRIVATE, TokenType::FINAL
			});
	}

	// Check if token is an operator
	bool Token::isOperator() const {
		return isOneOf({
			TokenType::PLUS, TokenType::MINUS, TokenType::STAR, TokenType::SLASH, TokenType::PERCENT,
			TokenType::EQUAL, TokenType::EQUAL_EQUAL, TokenType::NOT_EQUAL,
			TokenType::LESS, TokenType::LESS_EQUAL, TokenType::GREATER, TokenType::GREATER_EQUAL,
			TokenType::AND, TokenType::OR, TokenType::NOT, TokenType::ASSIGN,
			TokenType::PLUS_PLUS, TokenType::MINUS_MINUS, TokenType::PLUS_EQUAL, TokenType::MINUS_EQUAL
			});
	}

	// Check if token is a type keyword
	bool Token::isTypeKeyword() const {
		return isOneOf({ TokenType::INT, TokenType::FLOAT, TokenType::BOOL,
						TokenType::STRING, TokenType::VOID });
	}

	// Check if token is an access modifier
	bool Token::isAccessModifier() const {
		return isOneOf({ TokenType::PUBLIC, TokenType::PRIVATE });
	}
}
