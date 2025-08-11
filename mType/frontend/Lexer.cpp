#include "Lexer.hpp"
#include <cctype>

namespace mtype::frontend {
	// Static members
	std::map<std::string, TokenType> Lexer::keywords;
	bool Lexer::keywordsInitialized = false;

	Lexer::Lexer(const std::string& src, const std::string& file)
		: source(src), filename(file), start(0), current(0), line(1), column(1) {
		if (!keywordsInitialized) {
			initializeKeywords();
		}
	}

	// Initialize keywords map
	void Lexer::initializeKeywords() {
		keywords = {
			// Control flow
			{"if", TokenType::IF},
			{"else", TokenType::ELSE},
			{"switch", TokenType::SWITCH},
			{"case", TokenType::CASE},
			{"default", TokenType::DEFAULT},
			{"while", TokenType::WHILE},
			{"for", TokenType::FOR},
			{"do", TokenType::DO},
			{"break", TokenType::BREAK},
			{"continue", TokenType::CONTINUE},

			// Functions and classes
			{"function", TokenType::FUNCTION},
			{"return", TokenType::RETURN},
			{"class", TokenType::CLASS},
			{"new", TokenType::NEW},
			{"this", TokenType::THIS},
			{"enum", TokenType::ENUM},
			{"extends", TokenType::EXTENDS},
			{"super", TokenType::SUPER},
			{"constructor", TokenType::CONSTRUCTOR},

			// Types
			{"int", TokenType::INT},
			{"float", TokenType::FLOAT},
			{"bool", TokenType::BOOL},
			{"string", TokenType::STRING},
			{"void", TokenType::VOID},

			// Literals
			{"true", TokenType::TRUE},
			{"false", TokenType::FALSE},
			{"null", TokenType::NULL_TOKEN},

			// Modifiers
			{"public", TokenType::PUBLIC},
			{"private", TokenType::PRIVATE},
			{"final", TokenType::FINAL},
			{"static", TokenType::STATIC},

			// Modules and namespaces
			{"module", TokenType::MODULE},
			{"namespace", TokenType::NAMESPACE},
			{"using", TokenType::USING}
		};
		keywordsInitialized = true;
	}

	// Main scanning method
	Result<std::vector<Token>> Lexer::scanTokens() {
		tokens.clear();
		start = 0;
		current = 0;
		line = 1;
		column = 1;

		while (!isAtEnd()) {
			start = current;
			auto result = scanToken();
			if (result.isError()) {
				return Result<std::vector<Token>>::err(result.error());
			}
		}

		// Add EOF token
		tokens.emplace_back(TokenType::END_OF_FILE, std::string(""), line, column);
		return Result<std::vector<Token>>::ok(tokens);
	}

	// Helper methods
	bool Lexer::isAtEnd() const {
		return current >= source.length();
	}

	char Lexer::advance() {
		if (isAtEnd()) return '\0';

		char c = source[current++];
		if (c == '\n') {
			line++;
			column = 1;
		}
		else {
			column++;
		}
		return c;
	}

	char Lexer::peek() const {
		if (isAtEnd()) return '\0';
		return source[current];
	}

	char Lexer::peekNext() const {
		if (current + 1 >= source.length()) return '\0';
		return source[current + 1];
	}

	bool Lexer::match(char expected) {
		if (isAtEnd()) return false;
		if (source[current] != expected) return false;
		current++;
		column++;
		return true;
	}

	void Lexer::skipWhitespace() {
		while (!isAtEnd()) {
			char c = peek();
			if (c == ' ' || c == '\r' || c == '\t') {
				advance();
			}
			else if (c == '\n') {
				advance();
				// Optionally emit NEWLINE tokens for languages that care about them
				// addToken(TokenType::NEWLINE);
				// break;
			}
			else {
				break;
			}
		}
	}

	void Lexer::addToken(TokenType type) {
		std::string text = source.substr(start, current - start);
		int tokenColumn = column - static_cast<int>(current - start);
		tokens.emplace_back(type, text, line, tokenColumn);
	}

	void Lexer::addToken(TokenType type, int intValue) {
		std::string text = source.substr(start, current - start);
		int tokenColumn = column - static_cast<int>(current - start);
		tokens.emplace_back(Token::withIntLiteral(type, text, intValue, line, tokenColumn));
	}

	void Lexer::addToken(TokenType type, double doubleValue) {
		std::string text = source.substr(start, current - start);
		int tokenColumn = column - static_cast<int>(current - start);
		tokens.emplace_back(Token::withDoubleLiteral(type, text, doubleValue, line, tokenColumn));
	}

	void Lexer::addToken(TokenType type, const std::string& stringValue) {
		std::string text = source.substr(start, current - start);
		int tokenColumn = column - static_cast<int>(current - start);
		tokens.emplace_back(Token::withStringLiteral(type, text, stringValue, line, tokenColumn));
	}

	void Lexer::addToken(TokenType type, bool boolValue) {
		std::string text = source.substr(start, current - start);
		int tokenColumn = column - static_cast<int>(current - start);
		tokens.emplace_back(Token::withBoolLiteral(type, text, boolValue, line, tokenColumn));
	}

	void Lexer::addToken(TokenType type, bool boolValue) {
		std::string text = source.substr(start, current - start);
		tokens.emplace_back(type, text, boolValue, line, column - (current - start));
	}

	// Main token scanning
	Result<void> Lexer::scanToken() {
		skipWhitespace();
		if (isAtEnd()) return Result<void>::ok();

		start = current;
		char c = advance();

		switch (c) {
			// Single-character tokens
		case '(': addToken(TokenType::LPAREN); break;
		case ')': addToken(TokenType::RPAREN); break;
		case '{': addToken(TokenType::LBRACE); break;
		case '}': addToken(TokenType::RBRACE); break;
		case '[': addToken(TokenType::LBRACKET); break;
		case ']': addToken(TokenType::RBRACKET); break;
		case ',': addToken(TokenType::COMMA); break;
		case '.': addToken(TokenType::DOT); break;
		case ';': addToken(TokenType::SEMICOLON); break;
		case '*': addToken(TokenType::STAR); break;
		case '%': addToken(TokenType::PERCENT); break;

			// Two-character tokens or single character tokens
		case '+':
			if (match('+')) {
				addToken(TokenType::PLUS_PLUS);
			}
			else if (match('=')) {
				addToken(TokenType::PLUS_EQUAL);
			}
			else {
				addToken(TokenType::PLUS);
			}
			break;

		case '-':
			if (match('-')) {
				addToken(TokenType::MINUS_MINUS);
			}
			else if (match('=')) {
				addToken(TokenType::MINUS_EQUAL);
			}
			else {
				addToken(TokenType::MINUS);
			}
			break;

		case '!':
			addToken(match('=') ? TokenType::NOT_EQUAL : TokenType::NOT);
			break;

		case '=':
			addToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
			break;

		case '<':
			addToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
			break;

		case '>':
			addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
			break;

		case '&':
			if (match('&')) {
				addToken(TokenType::AND);
			}
			else {
				return Result<void>::err(makeError("Unexpected character '&'. Did you mean '&&'?"));
			}
			break;

		case '|':
			if (match('|')) {
				addToken(TokenType::OR);
			}
			else {
				return Result<void>::err(makeError("Unexpected character '|'. Did you mean '||'?"));
			}
			break;

		case ':':
			if (match(':')) {
				addToken(TokenType::DOUBLE_COLON);
			}
			else {
				addToken(TokenType::COLON);
			}
			break;

		case '/':
			if (match('/')) {
				// Single line comment
				auto result = scanSingleLineComment();
				if (result.isError()) return result;
			}
			else if (match('*')) {
				// Multi-line comment
				auto result = scanMultiLineComment();
				if (result.isError()) return result;
			}
			else {
				addToken(TokenType::SLASH);
			}
			break;

		case '"':
			return scanString();

		case '\n':
			// Optionally handle newlines as tokens
			addToken(TokenType::NEWLINE);
			break;

		default:
			if (isDigit(c)) {
				return scanNumber();
			}
			else if (isAlpha(c)) {
				return scanIdentifier();
			}
			else {
				return Result<void>::err(makeError("Unexpected character: " + std::string(1, c)));
			}
		}

		return Result<void>::ok();
	}

	// String scanning
	Result<void> Lexer::scanString() {
		std::string value;

		while (peek() != '"' && !isAtEnd()) {
			if (peek() == '\n') {
				line++;
				column = 1;
			}

			char c = advance();

			// Handle escape sequences
			if (c == '\\') {
				if (isAtEnd()) {
					return Result<void>::err(makeError("Unterminated string literal"));
				}

				char escaped = advance();
				switch (escaped) {
				case 'n': value += '\n'; break;
				case 't': value += '\t'; break;
				case 'r': value += '\r'; break;
				case '\\': value += '\\'; break;
				case '"': value += '"'; break;
				case '0': value += '\0'; break;
				default:
					return Result<void>::err(makeError("Invalid escape sequence: \\" + std::string(1, escaped)));
				}
			}
			else {
				value += c;
			}
		}

		if (isAtEnd()) {
			return Result<void>::err(makeError("Unterminated string literal"));
		}

		// Consume closing quote
		advance();

		addToken(TokenType::STRING_LITERALS, value);
		return Result<void>::ok();
	}

	// Number scanning
	Result<void> Lexer::scanNumber() {
		while (isDigit(peek())) {
			advance();
		}

		// Look for decimal part
		bool isFloat = false;
		if (peek() == '.' && isDigit(peekNext())) {
			isFloat = true;
			advance(); // consume '.'

			while (isDigit(peek())) {
				advance();
			}
		}

		std::string numberStr = source.substr(start, current - start);

		try {
			if (isFloat) {
				double value = std::stod(numberStr);
				addToken(TokenType::NUMBER, value);
			}
			else {
				int value = std::stoi(numberStr);
				addToken(TokenType::NUMBER, value);
			}
		}
		catch (const std::exception&) {
			return Result<void>::err(makeError("Invalid number format: " + numberStr));
		}

		return Result<void>::ok();
	}

	// Identifier and keyword scanning
	Result<void> Lexer::scanIdentifier() {
		while (isAlphaNumeric(peek())) {
			advance();
		}

		std::string text = source.substr(start, current - start);

		// Check if it's a keyword
		auto it = keywords.find(text);
		TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;

		// Handle boolean literals
		if (type == TokenType::TRUE) {
			addToken(type, true);
		}
		else if (type == TokenType::FALSE) {
			addToken(type, false);
		}
		else {
			addToken(type);
		}

		return Result<void>::ok();
	}

	// Comment scanning
	Result<void> Lexer::scanSingleLineComment() {
		while (peek() != '\n' && !isAtEnd()) {
			advance();
		}
		return Result<void>::ok();
	}

	Result<void> Lexer::scanMultiLineComment() {
		int nestLevel = 1;

		while (nestLevel > 0 && !isAtEnd()) {
			if (peek() == '/' && peekNext() == '*') {
				advance(); // consume '/'
				advance(); // consume '*'
				nestLevel++;
			}
			else if (peek() == '*' && peekNext() == '/') {
				advance(); // consume '*'
				advance(); // consume '/'
				nestLevel--;
			}
			else {
				advance();
			}
		}

		if (nestLevel > 0) {
			return Result<void>::err(makeError("Unterminated multi-line comment"));
		}

		return Result<void>::ok();
	}

	// Character classification
	bool Lexer::isDigit(char c) {
		return c >= '0' && c <= '9';
	}

	bool Lexer::isAlpha(char c) {
		return (c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			c == '_';
	}

	bool Lexer::isAlphaNumeric(char c) {
		return isAlpha(c) || isDigit(c);
	}

	// Error reporting
	Error Lexer::makeError(const std::string& message) const {
		return Error::syntax(message, line);
	}

	// Reset lexer with new source
	void Lexer::reset(const std::string& src, const std::string& file) {
		source = src;
		filename = file;
		tokens.clear();
		start = 0;
		current = 0;
		line = 1;
		column = 1;
	}

	// Utility methods
	bool Lexer::isKeyword(const std::string& word) {
		if (!keywordsInitialized) {
			const_cast<Lexer*>(nullptr)->initializeKeywords();
		}
		return keywords.find(word) != keywords.end();
	}

	TokenType Lexer::getKeywordType(const std::string& word) {
		if (!keywordsInitialized) {
			const_cast<Lexer*>(nullptr)->initializeKeywords();
		}
		auto it = keywords.find(word);
		return (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
	}
}
