#pragma once

#include "../../frontend/Lexer.hpp"
#include <iostream>
#include <cassert>
#include <iomanip>

using namespace mtype::frontend;
using namespace mtype::core;

namespace tests::runtime {
    class LexerTest {
    private:
        int testsPassed = 0;
        int testsFailed = 0;
        bool verbose = false;

        void printTestResult(const std::string& testName, bool passed) {
            if (passed) {
                std::cout << "[PASS] " << testName << std::endl;
                testsPassed++;
            }
            else {
                std::cout << "[FAIL] " << testName << std::endl;
                testsFailed++;
            }
        }

		void debugPrintTokens(const std::vector<Token>& tokens) {
			std::cout << "Token stream (" << tokens.size() << " tokens):" << std::endl;
			for (size_t i = 0; i < tokens.size(); i++) {
				const Token& t = tokens[i];
				std::cout << "  [" << i << "] "
					<< Token::typeToString(t.getType())
					<< " : '" << t.getLexeme() << "'"
					<< " (Line " << t.getLine() << ", Col " << t.getColumn() << ")";

				// Print literal values if present
				int intVal;
				if (t.getIntLiteral(intVal)) {
					std::cout << " = " << intVal;
				}

				std::cout << std::endl;
			}
		}

        bool compareTokens(const Token& expected, const Token& actual) {
            if (expected.getType() != actual.getType()) {
                if (verbose) {
                    std::cout << "  Type mismatch: expected " << Token::typeToString(expected.getType())
                        << ", got " << Token::typeToString(actual.getType()) << std::endl;
                }
                return false;
            }

            if (expected.getLexeme() != actual.getLexeme()) {
                if (verbose) {
                    std::cout << "  Lexeme mismatch: expected '" << expected.getLexeme()
                        << "', got '" << actual.getLexeme() << "'" << std::endl;
                }
                return false;
            }

            // Compare literals if needed
            int intVal1, intVal2;
            if (expected.getIntLiteral(intVal1) && actual.getIntLiteral(intVal2)) {
                if (intVal1 != intVal2) {
                    if (verbose) {
                        std::cout << "  Int literal mismatch: expected " << intVal1
                            << ", got " << intVal2 << std::endl;
                    }
                    return false;
                }
            }

            double doubleVal1, doubleVal2;
            if (expected.getDoubleLiteral(doubleVal1) && actual.getDoubleLiteral(doubleVal2)) {
                if (std::abs(doubleVal1 - doubleVal2) > 0.0001) {
                    if (verbose) {
                        std::cout << "  Double literal mismatch: expected " << doubleVal1
                            << ", got " << doubleVal2 << std::endl;
                    }
                    return false;
                }
            }

            std::string strVal1, strVal2;
            if (expected.getStringLiteral(strVal1) && actual.getStringLiteral(strVal2)) {
                if (strVal1 != strVal2) {
                    if (verbose) {
                        std::cout << "  String literal mismatch: expected \"" << strVal1
                            << "\", got \"" << strVal2 << "\"" << std::endl;
                    }
                    return false;
                }
            }

            bool boolVal1, boolVal2;
            if (expected.getBoolLiteral(boolVal1) && actual.getBoolLiteral(boolVal2)) {
                if (boolVal1 != boolVal2) {
                    if (verbose) {
                        std::cout << "  Bool literal mismatch: expected " << (boolVal1 ? "true" : "false")
                            << ", got " << (boolVal2 ? "true" : "false") << std::endl;
                    }
                    return false;
                }
            }

            return true;
        }

    public:
        void setVerbose(bool v) { verbose = v; }

        // Test single character tokens
        void testSingleCharTokens() {
            std::cout << "\n=== Testing Single Character Tokens ===" << std::endl;

            struct TestCase {
                std::string input;
                TokenType expectedType;
                std::string expectedLexeme;
            };

            TestCase cases[] = {
                {"(", TokenType::LPAREN, "("},
                {")", TokenType::RPAREN, ")"},
                {"{", TokenType::LBRACE, "{"},
                {"}", TokenType::RBRACE, "}"},
                {"[", TokenType::LBRACKET, "["},
                {"]", TokenType::RBRACKET, "]"},
                {",", TokenType::COMMA, ","},
                {".", TokenType::DOT, "."},
                {";", TokenType::SEMICOLON, ";"},
                {"*", TokenType::STAR, "*"},
                {"%", TokenType::PERCENT, "%"},
                {"+", TokenType::PLUS, "+"},
                {"-", TokenType::MINUS, "-"},
                {"/", TokenType::SLASH, "/"},
                {"!", TokenType::NOT, "!"},
                {"=", TokenType::EQUAL, "="},
                {"<", TokenType::LESS, "<"},
                {">", TokenType::GREATER, ">"},
                {":", TokenType::COLON, ":"}
            };

            for (const auto& tc : cases) {
                Lexer lexer(tc.input);
                auto result = lexer.scanTokens();

                if (result.isError()) {
                    printTestResult("Single char '" + tc.input + "'", false);
                    if (verbose) std::cout << "  Error: " << result.error().toString() << std::endl;
                    continue;
                }

                auto tokens = result.value();
                bool passed = tokens.size() == 2 && // token + EOF
                    tokens[0].getType() == tc.expectedType &&
                    tokens[0].getLexeme() == tc.expectedLexeme;

                printTestResult("Single char '" + tc.input + "'", passed);
            }
        }

        // Test two-character tokens
        void testTwoCharTokens() {
            std::cout << "\n=== Testing Two Character Tokens ===" << std::endl;

            struct TestCase {
                std::string input;
                TokenType expectedType;
                std::string expectedLexeme;
            };

            TestCase cases[] = {
                {"++", TokenType::PLUS_PLUS, "++"},
                {"--", TokenType::MINUS_MINUS, "--"},
                {"+=", TokenType::PLUS_EQUAL, "+="},
                {"-=", TokenType::MINUS_EQUAL, "-="},
                {"==", TokenType::EQUAL_EQUAL, "=="},
                {"!=", TokenType::NOT_EQUAL, "!="},
                {"<=", TokenType::LESS_EQUAL, "<="},
                {">=", TokenType::GREATER_EQUAL, ">="},
                {"&&", TokenType::AND, "&&"},
                {"||", TokenType::OR, "||"},
                {"::", TokenType::DOUBLE_COLON, "::"}
            };

            for (const auto& tc : cases) {
                Lexer lexer(tc.input);
                auto result = lexer.scanTokens();

                if (result.isError()) {
                    printTestResult("Two char '" + tc.input + "'", false);
                    continue;
                }

                auto tokens = result.value();
                bool passed = tokens.size() == 2 &&
                    tokens[0].getType() == tc.expectedType &&
                    tokens[0].getLexeme() == tc.expectedLexeme;

                printTestResult("Two char '" + tc.input + "'", passed);
            }
        }

        // Test keywords
        void testKeywords() {
            std::cout << "\n=== Testing Keywords ===" << std::endl;

            struct TestCase {
                std::string input;
                TokenType expectedType;
            };

            TestCase cases[] = {
                {"if", TokenType::IF},
                {"else", TokenType::ELSE},
                {"while", TokenType::WHILE},
                {"for", TokenType::FOR},
                {"function", TokenType::FUNCTION},
                {"return", TokenType::RETURN},
                {"class", TokenType::CLASS},
                {"new", TokenType::NEW},
                {"this", TokenType::THIS},
                {"true", TokenType::TRUE},
                {"false", TokenType::FALSE},
                {"null", TokenType::NULL_TOKEN},
                {"int", TokenType::INT},
                {"float", TokenType::FLOAT},
                {"bool", TokenType::BOOL},
                {"string", TokenType::STRING},
                {"void", TokenType::VOID},
                {"public", TokenType::PUBLIC},
                {"private", TokenType::PRIVATE},
                {"static", TokenType::STATIC},
                {"namespace", TokenType::NAMESPACE},
                {"using", TokenType::USING}
            };

            for (const auto& tc : cases) {
                Lexer lexer(tc.input);
                auto result = lexer.scanTokens();

                if (result.isError()) {
                    printTestResult("Keyword '" + tc.input + "'", false);
                    continue;
                }

                auto tokens = result.value();
                bool passed = tokens.size() == 2 &&
                    tokens[0].getType() == tc.expectedType;

                printTestResult("Keyword '" + tc.input + "'", passed);
            }
        }

        // Test identifiers
        void testIdentifiers() {
            std::cout << "\n=== Testing Identifiers ===" << std::endl;

            std::string cases[] = {
                "variable",
                "myVar",
                "MY_CONSTANT",
                "_private",
                "var123",
                "camelCase",
                "snake_case",
                "_123"
            };

            for (const auto& input : cases) {
                Lexer lexer(input);
                auto result = lexer.scanTokens();

                if (result.isError()) {
                    printTestResult("Identifier '" + input + "'", false);
                    continue;
                }

                auto tokens = result.value();
                bool passed = tokens.size() == 2 &&
                    tokens[0].getType() == TokenType::IDENTIFIER &&
                    tokens[0].getLexeme() == input;

                printTestResult("Identifier '" + input + "'", passed);
            }
        }

        // Test numbers
        void testNumbers() {
            std::cout << "\n=== Testing Numbers ===" << std::endl;

            struct TestCase {
                std::string input;
                bool isFloat;
                double expectedValue;
            };

            TestCase cases[] = {
                {"0", false, 0},
                {"123", false, 123},
                {"456789", false, 456789},
                {"3.14", true, 3.14},
                {"0.0", true, 0.0},
                {"123.456", true, 123.456},
                {"1.0", true, 1.0}
            };

            for (const auto& tc : cases) {
                Lexer lexer(tc.input);
                auto result = lexer.scanTokens();

                if (result.isError()) {
                    printTestResult("Number '" + tc.input + "'", false);
                    continue;
                }

                auto tokens = result.value();
                if (tokens.size() != 2 || tokens[0].getType() != TokenType::NUMBER) {
                    printTestResult("Number '" + tc.input + "'", false);
                    continue;
                }

                bool passed = false;
                if (tc.isFloat) {
                    double val;
                    if (tokens[0].getDoubleLiteral(val)) {
                        passed = std::abs(val - tc.expectedValue) < 0.0001;
                    }
                }
                else {
                    int val;
                    if (tokens[0].getIntLiteral(val)) {
                        passed = val == static_cast<int>(tc.expectedValue);
                    }
                }

                printTestResult("Number '" + tc.input + "'", passed);
            }
        }

        // Test strings
        void testStrings() {
            std::cout << "\n=== Testing Strings ===" << std::endl;

            struct TestCase {
                std::string input;
                std::string expectedValue;
            };

            TestCase cases[] = {
                {"\"\"", ""},
                {"\"hello\"", "hello"},
                {"\"Hello, World!\"", "Hello, World!"},
                {"\"with spaces\"", "with spaces"},
                {"\"escape\\nnewline\"", "escape\nnewline"},
                {"\"tab\\there\"", "tab\there"},
                {"\"quote\\\"inside\"", "quote\"inside"},
                {"\"backslash\\\\here\"", "backslash\\here"}
            };

            for (const auto& tc : cases) {
                Lexer lexer(tc.input);
                auto result = lexer.scanTokens();

                if (result.isError()) {
                    printTestResult("String " + tc.input, false);
                    if (verbose) std::cout << "  Error: " << result.error().toString() << std::endl;
                    continue;
                }

                auto tokens = result.value();
                if (tokens.size() != 2 || tokens[0].getType() != TokenType::STRING_LITERALS) {
                    printTestResult("String " + tc.input, false);
                    continue;
                }

                std::string val;
                bool passed = tokens[0].getStringLiteral(val) && val == tc.expectedValue;

                printTestResult("String " + tc.input, passed);
            }
        }

        // Test comments
        void testComments() {
            std::cout << "\n=== Testing Comments ===" << std::endl;

            // Single line comments
            {
                std::string input = "123 // this is a comment\n456";
                Lexer lexer(input);
                auto result = lexer.scanTokens();

                bool passed = false;
                if (result.isOk()) {
                    auto tokens = result.value();
                    // Should have: 123, NEWLINE, 456, EOF
                    passed = tokens.size() == 4 &&
                        tokens[0].getType() == TokenType::NUMBER &&
                        tokens[1].getType() == TokenType::NEWLINE &&
                        tokens[2].getType() == TokenType::NUMBER;
                }

                printTestResult("Single line comment", passed);
            }

            // Multi-line comments
            {
                std::string input = "123 /* this is\na multi-line\ncomment */ 456";
                Lexer lexer(input);
                auto result = lexer.scanTokens();

                bool passed = false;
                if (result.isOk()) {
                    auto tokens = result.value();
                    // Should have: 123, 456, EOF
                    passed = tokens.size() == 3 &&
                        tokens[0].getType() == TokenType::NUMBER &&
                        tokens[1].getType() == TokenType::NUMBER;
                }

                printTestResult("Multi-line comment", passed);
            }

            // Nested multi-line comments
            {
                std::string input = "123 /* outer /* inner */ still comment */ 456";
                Lexer lexer(input);
                auto result = lexer.scanTokens();

                bool passed = false;
                if (result.isOk()) {
                    auto tokens = result.value();
                    passed = tokens.size() == 3 &&
                        tokens[0].getType() == TokenType::NUMBER &&
                        tokens[1].getType() == TokenType::NUMBER;
                }

                printTestResult("Nested multi-line comment", passed);
            }
        }

        // Test complex expressions
        void testComplexExpressions() {
            std::cout << "\n=== Testing Complex Expressions ===" << std::endl;

            // Mathematical expression
            {
                std::string input = "3 + 4 * 2 - 1";
                Lexer lexer(input);
                auto result = lexer.scanTokens();

                bool passed = false;
                if (result.isOk()) {
                    auto tokens = result.value();
                    passed = tokens.size() == 8 && // 3, +, 4, *, 2, -, 1, EOF
                        tokens[0].getType() == TokenType::NUMBER &&
                        tokens[1].getType() == TokenType::PLUS &&
                        tokens[2].getType() == TokenType::NUMBER &&
                        tokens[3].getType() == TokenType::STAR &&
                        tokens[4].getType() == TokenType::NUMBER &&
                        tokens[5].getType() == TokenType::MINUS &&
                        tokens[6].getType() == TokenType::NUMBER;
                }

                printTestResult("Mathematical expression", passed);
            }

            // Boolean expression
            {
                std::string input = "x >= 10 && y != 20 || z == true";
                Lexer lexer(input);
                auto result = lexer.scanTokens();

                bool passed = false;
                if (result.isOk()) {
                    auto tokens = result.value();
                    passed = tokens.size() == 12 && // x, >=, 10, &&, y, !=, 20, ||, z, ==, true, EOF
                        tokens[0].getType() == TokenType::IDENTIFIER &&
                        tokens[1].getType() == TokenType::GREATER_EQUAL &&
                        tokens[2].getType() == TokenType::NUMBER &&
                        tokens[3].getType() == TokenType::AND &&
                        tokens[4].getType() == TokenType::IDENTIFIER &&
                        tokens[5].getType() == TokenType::NOT_EQUAL &&
                        tokens[6].getType() == TokenType::NUMBER &&
                        tokens[7].getType() == TokenType::OR &&
                        tokens[8].getType() == TokenType::IDENTIFIER &&
                        tokens[9].getType() == TokenType::EQUAL_EQUAL &&
                        tokens[10].getType() == TokenType::TRUE;
                }

                printTestResult("Boolean expression", passed);
            }

            // Function call
            {
                std::string input = "function test(int x, string y) { return x + 1; }";
                Lexer lexer(input);
                auto result = lexer.scanTokens();

                bool passed = false;
                if (result.isOk()) {
                    auto tokens = result.value();
                    passed = tokens[0].getType() == TokenType::FUNCTION &&
                        tokens[1].getType() == TokenType::IDENTIFIER &&
                        tokens[2].getType() == TokenType::LPAREN &&
                        tokens[3].getType() == TokenType::INT;
                    // ... more checks could be added
                }

                printTestResult("Function declaration", passed);
            }
        }

        // Test error cases
        void testErrorCases() {
            std::cout << "\n=== Testing Error Cases ===" << std::endl;

            // Unterminated string
            {
                std::string input = "\"unterminated string";
                Lexer lexer(input);
                auto result = lexer.scanTokens();

                bool passed = result.isError() &&
                    result.error().type == ErrorType::SYNTAX_ERROR;

                printTestResult("Unterminated string error", passed);
            }

            // Invalid escape sequence
            {
                std::string input = "\"invalid\\x escape\"";
                Lexer lexer(input);
                auto result = lexer.scanTokens();

                bool passed = result.isError() &&
                    result.error().type == ErrorType::SYNTAX_ERROR;

                printTestResult("Invalid escape sequence error", passed);
            }

            // Unterminated multi-line comment
            {
                std::string input = "/* unterminated comment";
                Lexer lexer(input);
                auto result = lexer.scanTokens();

                bool passed = result.isError() &&
                    result.error().type == ErrorType::SYNTAX_ERROR;

                printTestResult("Unterminated comment error", passed);
            }

            // Single & (should be &&)
            {
                std::string input = "a & b";
                Lexer lexer(input);
                auto result = lexer.scanTokens();

                bool passed = result.isError() &&
                    result.error().type == ErrorType::SYNTAX_ERROR;

                printTestResult("Single & error", passed);
            }

            // Single | (should be ||)
            {
                std::string input = "a | b";
                Lexer lexer(input);
                auto result = lexer.scanTokens();

                bool passed = result.isError() &&
                    result.error().type == ErrorType::SYNTAX_ERROR;

                printTestResult("Single | error", passed);
            }
        }

        // Test whitespace handling
        void testWhitespaceHandling() {
            std::cout << "\n=== Testing Whitespace Handling ===" << std::endl;

            // Multiple spaces
            {
                std::string input = "a    +    b";
                Lexer lexer(input);
                auto result = lexer.scanTokens();

                bool passed = false;
                if (result.isOk()) {
                    auto tokens = result.value();
                    passed = tokens.size() == 4 && // a, +, b, EOF
                        tokens[0].getType() == TokenType::IDENTIFIER &&
                        tokens[1].getType() == TokenType::PLUS &&
                        tokens[2].getType() == TokenType::IDENTIFIER;
                }

                printTestResult("Multiple spaces", passed);
            }

            // Tabs and newlines
            {
                std::string input = "a\t+\n\tb";
                Lexer lexer(input);
                auto result = lexer.scanTokens();

                bool passed = false;
                if (result.isOk()) {
                    auto tokens = result.value();
                    // Should have: a, +, NEWLINE, b, EOF
                    passed = tokens.size() == 5 &&
                        tokens[0].getType() == TokenType::IDENTIFIER &&
                        tokens[1].getType() == TokenType::PLUS &&
                        tokens[2].getType() == TokenType::NEWLINE &&
                        tokens[3].getType() == TokenType::IDENTIFIER;
                }

                printTestResult("Tabs and newlines", passed);
            }
        }

        // Test position tracking
        void testPositionTracking() {
            std::cout << "\n=== Testing Position Tracking ===" << std::endl;

            std::string input = "int x = 5;\nstring y = \"hello\";";
            Lexer lexer(input);
            auto result = lexer.scanTokens();

            bool passed = false;
            if (result.isOk()) {
                auto tokens = result.value();

                // Check line numbers
                passed = tokens[0].getLine() == 1 && // int
                    tokens[4].getLine() == 1 && // semicolon
                    tokens[6].getLine() == 2;   // string
            }

            printTestResult("Line number tracking", passed);
        }

		// Test class-related keywords and constructs
		void testClassKeywords() {
			std::cout << "\n=== Testing Class-Related Keywords ===" << std::endl;

			struct TestCase {
				std::string input;
				std::vector<TokenType> expectedTypes;
				std::string description;
			};

			TestCase cases[] = {
				{
					"class extends super",
					{TokenType::CLASS, TokenType::EXTENDS, TokenType::SUPER, TokenType::END_OF_FILE},
					"Class inheritance keywords"
				},
				{
					"constructor final static",
					{TokenType::CONSTRUCTOR, TokenType::FINAL, TokenType::STATIC, TokenType::END_OF_FILE},
					"Class modifier keywords"
				},
				{
					"public private this",
					{TokenType::PUBLIC, TokenType::PRIVATE, TokenType::THIS, TokenType::END_OF_FILE},
					"Access modifiers and this"
				},
				{
					"new MyClass()",
					{TokenType::NEW, TokenType::IDENTIFIER, TokenType::LPAREN, TokenType::RPAREN, TokenType::END_OF_FILE},
					"Object instantiation"
				},
				{
					"this.field",
					{TokenType::THIS, TokenType::DOT, TokenType::IDENTIFIER, TokenType::END_OF_FILE},
					"This member access"
				},
				{
					"super.method()",
					{TokenType::SUPER, TokenType::DOT, TokenType::IDENTIFIER, TokenType::LPAREN, TokenType::RPAREN, TokenType::END_OF_FILE},
					"Super method call"
				},
				{
					"class Animal extends Creature",
					{TokenType::CLASS, TokenType::IDENTIFIER, TokenType::EXTENDS, TokenType::IDENTIFIER, TokenType::END_OF_FILE},
					"Class declaration with inheritance"
				}
			};

			for (const auto& tc : cases) {
				Lexer lexer(tc.input);
				auto result = lexer.scanTokens();

				if (result.isError()) {
					printTestResult("Class: " + tc.description, false);
					if (verbose) std::cout << "  Error: " << result.error().toString() << std::endl;
					continue;
				}

				auto tokens = result.value();
				bool passed = tokens.size() == tc.expectedTypes.size();

				if (passed) {
					for (size_t i = 0; i < tokens.size(); i++) {
						if (tokens[i].getType() != tc.expectedTypes[i]) {
							passed = false;
							if (verbose) {
								std::cout << "  Token " << i << " mismatch: expected "
									<< Token::typeToString(tc.expectedTypes[i])
									<< ", got " << Token::typeToString(tokens[i].getType()) << std::endl;
							}
							break;
						}
					}
				}

				printTestResult("Class: " + tc.description, passed);
			}
		}

		// Test namespace-related keywords and constructs
		void testNamespaceKeywords() {
			std::cout << "\n=== Testing Namespace-Related Keywords ===" << std::endl;

			struct TestCase {
				std::string input;
				std::vector<TokenType> expectedTypes;
				std::string description;
			};

			TestCase cases[] = {
				{
					"namespace using",
					{TokenType::NAMESPACE, TokenType::USING, TokenType::END_OF_FILE},
					"Namespace keywords"
				},
				{
					"namespace Math",
					{TokenType::NAMESPACE, TokenType::IDENTIFIER, TokenType::END_OF_FILE},
					"Namespace declaration"
				},
				{
					"using System",
					{TokenType::USING, TokenType::IDENTIFIER, TokenType::END_OF_FILE},
					"Using directive"
				},
				{
					"Math::PI",
					{TokenType::IDENTIFIER, TokenType::DOUBLE_COLON, TokenType::IDENTIFIER, TokenType::END_OF_FILE},
					"Namespace resolution"
				},
				{
					"System::Collections::List",
					{TokenType::IDENTIFIER, TokenType::DOUBLE_COLON, TokenType::IDENTIFIER,
					 TokenType::DOUBLE_COLON, TokenType::IDENTIFIER, TokenType::END_OF_FILE},
					"Nested namespace resolution"
				},
				{
					"using namespace std",
					{TokenType::USING, TokenType::NAMESPACE, TokenType::IDENTIFIER, TokenType::END_OF_FILE},
					"Using namespace directive"
				},
				{
					"::globalFunction",
					{TokenType::DOUBLE_COLON, TokenType::IDENTIFIER, TokenType::END_OF_FILE},
					"Global namespace access"
				}
			};

			for (const auto& tc : cases) {
				Lexer lexer(tc.input);
				auto result = lexer.scanTokens();

				if (result.isError()) {
					printTestResult("Namespace: " + tc.description, false);
					if (verbose) std::cout << "  Error: " << result.error().toString() << std::endl;
					continue;
				}

				auto tokens = result.value();
				bool passed = tokens.size() == tc.expectedTypes.size();

				if (passed) {
					for (size_t i = 0; i < tokens.size(); i++) {
						if (tokens[i].getType() != tc.expectedTypes[i]) {
							passed = false;
							if (verbose) {
								std::cout << "  Token " << i << " mismatch: expected "
									<< Token::typeToString(tc.expectedTypes[i])
									<< ", got " << Token::typeToString(tokens[i].getType()) << std::endl;
							}
							break;
						}
					}
				}

				printTestResult("Namespace: " + tc.description, passed);
			}
		}

		// Test enum-related constructs
		void testEnumKeywords() {
			std::cout << "\n=== Testing Enum-Related Keywords ===" << std::endl;

			struct TestCase {
				std::string input;
				std::vector<TokenType> expectedTypes;
				std::string description;
			};

			TestCase cases[] = {
				{
					"enum Color",
					{TokenType::ENUM, TokenType::IDENTIFIER, TokenType::END_OF_FILE},
					"Enum declaration"
				},
				{
					"enum Status { ACTIVE, INACTIVE }",
					{TokenType::ENUM, TokenType::IDENTIFIER, TokenType::LBRACE,
					 TokenType::IDENTIFIER, TokenType::COMMA, TokenType::IDENTIFIER,
					 TokenType::RBRACE, TokenType::END_OF_FILE},
					"Enum with values"
				},
				{
					"Color.RED",
					{TokenType::IDENTIFIER, TokenType::DOT, TokenType::IDENTIFIER, TokenType::END_OF_FILE},
					"Enum value access"
				}
			};

			for (const auto& tc : cases) {
				Lexer lexer(tc.input);
				auto result = lexer.scanTokens();

				if (result.isError()) {
					printTestResult("Enum: " + tc.description, false);
					continue;
				}

				auto tokens = result.value();
				bool passed = tokens.size() == tc.expectedTypes.size();

				if (passed) {
					for (size_t i = 0; i < tokens.size(); i++) {
						if (tokens[i].getType() != tc.expectedTypes[i]) {
							passed = false;
							break;
						}
					}
				}

				printTestResult("Enum: " + tc.description, passed);
			}
		}

		// Test module-related keywords
		void testModuleKeywords() {
			std::cout << "\n=== Testing Module-Related Keywords ===" << std::endl;

			struct TestCase {
				std::string input;
				std::vector<TokenType> expectedTypes;
				std::string description;
			};

			TestCase cases[] = {
				{
					"import MyModule",
					{TokenType::IMPORT, TokenType::IDENTIFIER, TokenType::END_OF_FILE},
					"Module declaration"
				},
				{
					"import utils.helpers",
					{TokenType::IMPORT, TokenType::IDENTIFIER, TokenType::DOT,
					 TokenType::IDENTIFIER, TokenType::END_OF_FILE},
					"Module with package path"
				}
			};

			for (const auto& tc : cases) {
				Lexer lexer(tc.input);
				auto result = lexer.scanTokens();

				if (result.isError()) {
					printTestResult("Module: " + tc.description, false);
					continue;
				}

				auto tokens = result.value();
				bool passed = tokens.size() == tc.expectedTypes.size();

				if (passed) {
					for (size_t i = 0; i < tokens.size(); i++) {
						if (tokens[i].getType() != tc.expectedTypes[i]) {
							passed = false;
							break;
						}
					}
				}

				printTestResult("Module: " + tc.description, passed);
			}
		}

		// Test control flow keywords
		void testControlFlowKeywords() {
			std::cout << "\n=== Testing Control Flow Keywords ===" << std::endl;

			struct TestCase {
				std::string input;
				std::vector<TokenType> expectedTypes;
				std::string description;
			};

			TestCase cases[] = {
				{
					"switch case default",
					{TokenType::SWITCH, TokenType::CASE, TokenType::DEFAULT, TokenType::END_OF_FILE},
					"Switch statement keywords"
				},
				{
					"do while",
					{TokenType::DO, TokenType::WHILE, TokenType::END_OF_FILE},
					"Do-while keywords"
				},
				{
					"break continue",
					{TokenType::BREAK, TokenType::CONTINUE, TokenType::END_OF_FILE},
					"Loop control keywords"
				},
				{
					"for (int i = 0; i < 10; i++)",
					{TokenType::FOR, TokenType::LPAREN, TokenType::INT, TokenType::IDENTIFIER,
					 TokenType::EQUAL, TokenType::NUMBER, TokenType::SEMICOLON, TokenType::IDENTIFIER,
					 TokenType::LESS, TokenType::NUMBER, TokenType::SEMICOLON, TokenType::IDENTIFIER,
					 TokenType::PLUS_PLUS, TokenType::RPAREN, TokenType::END_OF_FILE},
					"For loop structure"
				},
				{
					"switch (x) { case 1: break; default: }",
					{TokenType::SWITCH, TokenType::LPAREN, TokenType::IDENTIFIER, TokenType::RPAREN,
					 TokenType::LBRACE, TokenType::CASE, TokenType::NUMBER, TokenType::COLON,
					 TokenType::BREAK, TokenType::SEMICOLON, TokenType::DEFAULT, TokenType::COLON,
					 TokenType::RBRACE, TokenType::END_OF_FILE},
					"Switch statement structure"
				}
			};

			for (const auto& tc : cases) {
				Lexer lexer(tc.input);
				auto result = lexer.scanTokens();

				if (result.isError()) {
					printTestResult("Control flow: " + tc.description, false);
					continue;
				}

				auto tokens = result.value();
				bool passed = tokens.size() == tc.expectedTypes.size();

				if (passed) {
					for (size_t i = 0; i < tokens.size(); i++) {
						if (tokens[i].getType() != tc.expectedTypes[i]) {
							passed = false;
							if (verbose) {
								std::cout << "  Token " << i << " mismatch: expected "
									<< Token::typeToString(tc.expectedTypes[i])
									<< ", got " << Token::typeToString(tokens[i].getType()) << std::endl;
							}
							break;
						}
					}
				}

				printTestResult("Control flow: " + tc.description, passed);
			}
		}

		// Test complex class declarations
		void testComplexClassDeclarations() {
			std::cout << "\n=== Testing Complex Class Declarations ===" << std::endl;

			// Public static final method
			{
				std::string input = "public static final void process()";
				Lexer lexer(input);
				auto result = lexer.scanTokens();

				bool passed = false;
				if (result.isOk()) {
					auto tokens = result.value();
					passed = tokens.size() == 8 &&
						tokens[0].getType() == TokenType::PUBLIC &&
						tokens[1].getType() == TokenType::STATIC &&
						tokens[2].getType() == TokenType::FINAL &&
						tokens[3].getType() == TokenType::VOID &&
						tokens[4].getType() == TokenType::IDENTIFIER &&
						tokens[5].getType() == TokenType::LPAREN &&
						tokens[6].getType() == TokenType::RPAREN;
				}

				printTestResult("Complex class: public static final method", passed);
			}

			// Private constructor
			{
				std::string input = "private constructor(string name)";
				Lexer lexer(input);
				auto result = lexer.scanTokens();

				bool passed = false;
				if (result.isOk()) {
					auto tokens = result.value();
					passed = tokens.size() == 7 &&  // Fixed: should be 7 tokens including EOF
						tokens[0].getType() == TokenType::PRIVATE &&
						tokens[1].getType() == TokenType::CONSTRUCTOR &&
						tokens[2].getType() == TokenType::LPAREN &&
						tokens[3].getType() == TokenType::STRING &&
						tokens[4].getType() == TokenType::IDENTIFIER &&
						tokens[5].getType() == TokenType::RPAREN &&
						tokens[6].getType() == TokenType::END_OF_FILE;
				}

				printTestResult("Complex class: private constructor", passed);
			}

			// Class with extends and constructor calling super
			{
				std::string input = "class Dog extends Animal { constructor() { super(); } }";
				Lexer lexer(input);
				auto result = lexer.scanTokens();

				bool passed = false;
				if (result.isOk()) {
					auto tokens = result.value();
					// Check key tokens
					passed = tokens[0].getType() == TokenType::CLASS &&
						tokens[1].getType() == TokenType::IDENTIFIER &&
						tokens[2].getType() == TokenType::EXTENDS &&
						tokens[3].getType() == TokenType::IDENTIFIER &&
						tokens[4].getType() == TokenType::LBRACE;

					// Find super token
					bool foundSuper = false;
					for (const auto& token : tokens) {
						if (token.getType() == TokenType::SUPER) {
							foundSuper = true;
							break;
						}
					}
					passed = passed && foundSuper;
				}

				printTestResult("Complex class: inheritance with super call", passed);
			}
		}

        // Run all tests
        void runAllTests() {
            std::cout << "\n========================================" << std::endl;
            std::cout << "     mType Lexer Test Suite" << std::endl;
            std::cout << "========================================" << std::endl;

            testSingleCharTokens();
            testTwoCharTokens();
            testKeywords();
            testIdentifiers();
            testNumbers();
            testStrings();
            testComments();
            testComplexExpressions();
            testErrorCases();
            testWhitespaceHandling();
            testPositionTracking();
			testClassKeywords();
			testNamespaceKeywords();
			testEnumKeywords();
			testModuleKeywords();
			testControlFlowKeywords();
			testComplexClassDeclarations();

            std::cout << "\n========================================" << std::endl;
            std::cout << "Test Results:" << std::endl;
            std::cout << "  Passed: " << testsPassed << std::endl;
            std::cout << "  Failed: " << testsFailed << std::endl;
            std::cout << "  Total:  " << (testsPassed + testsFailed) << std::endl;
            std::cout << "  Success Rate: " << std::fixed << std::setprecision(1)
                << (testsPassed * 100.0 / (testsPassed + testsFailed)) << "%" << std::endl;
            std::cout << "========================================" << std::endl;
        }
    };
}

