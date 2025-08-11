#pragma once
#include "../core/Result.hpp"
#include "../core/Token.hpp"
#include <string>
#include <vector>
#include <map>

namespace mtype {

	using namespace core;
	namespace frontend {

		class Lexer {
		private:
			std::string source;
			std::string filename;
			std::vector<Token> tokens;
			size_t start;
			size_t current;
			int line;
			int column;

			static std::map<std::string, TokenType> keywords;
			static bool keywordsInitialized;

			// Helper methods
			bool isAtEnd() const;
			char advance();
			char peek() const;
			char peekNext() const;
			bool match(char expected);
			void skipWhitespace();

			// Token creation
			void addToken(TokenType type);
			void addToken(TokenType type, int intValue);
			void addToken(TokenType type, double doubleValue);
			void addToken(TokenType type, const std::string& stringValue);
			void addToken(TokenType type, bool boolValue);

			// Scanning methods
			Result<void> scanToken();
			Result<void> scanString();
			Result<void> scanNumber();
			Result<void> scanIdentifier();
			Result<void> scanSingleLineComment();
			Result<void> scanMultiLineComment();

			// Character classification
			static bool isDigit(char c);
			static bool isAlpha(char c);
			static bool isAlphaNumeric(char c);

			// Error reporting
			Error makeError(const std::string& message) const;

			// Initialize keywords map
			static void initializeKeywords();

		public:
			Lexer(const std::string& src, const std::string& file = "<input>");

			// Main scanning method
			Result<std::vector<Token>> scanTokens();

			// Get tokens (after scanning)
			const std::vector<Token>& getTokens() const { return tokens; }

			// Reset lexer with new source
			void reset(const std::string& src, const std::string& file = "<input>");

			// Utility methods
			static bool isKeyword(const std::string& word);
			static TokenType getKeywordType(const std::string& word);
		};
	}

} 

