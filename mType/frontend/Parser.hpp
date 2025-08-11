#pragma once
#include "../core/Result.hpp"
#include "../core/Token.hpp"
#include "../ast/AstNode.hpp"
#include <vector>
#include <memory>
#include <initializer_list>

namespace mtype::frontend {

	// Forward declarations
	class Interpreter;
	using namespace core;
	using namespace ast;
	
		class Parser {
		private:
			std::vector<Token> tokens;
			size_t current;
			std::string filename;
			Interpreter* interpreter;  // For module loading

			// Parser state
			bool inLoop;
			bool inFunction;
			bool inClass;
			int loopDepth;

			// Helper methods
			bool isAtEnd() const;
			Token peek() const;
			Token previous() const;
			Token advance();
			bool check(TokenType type) const;
			bool match(std::initializer_list<TokenType> types);
			bool match(TokenType type);

			// Error handling
			Result<Token> consume(TokenType type, const std::string& message);
			Error error(const Token& token, const std::string& message) const;
			Error error(const std::string& message) const;
			void synchronize();

			// Type parsing
			Result<ValueType> parseType();
			Result<std::vector<ValueType>> parseTypeArguments();

			// Declarations
			Result<ASTNodePtr> declaration();
			Result<ASTNodePtr> varDeclaration();
			Result<ASTNodePtr> functionDeclaration();
			Result<ASTNodePtr> classDeclaration();
			Result<ASTNodePtr> enumDeclaration();
			Result<ASTNodePtr> namespaceDeclaration();
			Result<ASTNodePtr> importStatement();
			Result<ASTNodePtr> exportStatement();
			Result<ASTNodePtr> usingDirective();

			// Class members
			Result<ASTNodePtr> classMethod(AccessModifier access, bool isStatic);
			Result<ASTNodePtr> classField(AccessModifier access, bool isStatic);
			Result<ASTNodePtr> classConstructor(AccessModifier access);

			// Statements
			Result<ASTNodePtr> statement();
			Result<ASTNodePtr> ifStatement();
			Result<ASTNodePtr> whileStatement();
			Result<ASTNodePtr> forStatement();
			Result<ASTNodePtr> returnStatement();
			Result<ASTNodePtr> breakStatement();
			Result<ASTNodePtr> continueStatement();
			Result<ASTNodePtr> blockStatement();
			Result<ASTNodePtr> expressionStatement();
			Result<ASTNodePtr> tryStatement();
			Result<ASTNodePtr> throwStatement();

			// Expressions (precedence climbing)
			Result<ASTNodePtr> expression();
			Result<ASTNodePtr> assignment();
			Result<ASTNodePtr> ternary();
			Result<ASTNodePtr> logicalOr();
			Result<ASTNodePtr> logicalAnd();
			Result<ASTNodePtr> equality();
			Result<ASTNodePtr> comparison();
			Result<ASTNodePtr> term();
			Result<ASTNodePtr> factor();
			Result<ASTNodePtr> unary();
			Result<ASTNodePtr> postfix();
			Result<ASTNodePtr> primary();

			// Special expression parsing
			Result<ASTNodePtr> callExpression(ASTNodePtr callee);
			Result<ASTNodePtr> memberAccess(ASTNodePtr object);
			Result<ASTNodePtr> indexAccess(ASTNodePtr array);
			Result<ASTNodePtr> newExpression();
			Result<ASTNodePtr> arrayLiteral();

			// Parameter parsing
			struct Parameter {
				ValueType type;
				std::string name;
				ASTNodePtr defaultValue;
			};
			Result<std::vector<Parameter>> parseParameters();
			Result<std::vector<ASTNodePtr>> parseArguments();

			// Access modifier parsing
			Result<AccessModifier> parseAccessModifier();

		public:
			Parser(const std::vector<Token>& t, const std::string& file = "<input>");

			// Set interpreter for module loading
			void setInterpreter(Interpreter* interp) { interpreter = interp; }

			// Main parsing method
			Result<std::vector<ASTNodePtr>> parse();

			// Parse single expression (for REPL)
			Result<ASTNodePtr> parseExpression();

			// Parse single statement
			Result<ASTNodePtr> parseStatement();

			// Reset parser with new tokens
			void reset(const std::vector<Token>& t, const std::string& file = "<input>");
		};
	
}
