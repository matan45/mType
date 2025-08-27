#include "../lexer/Lexer.hpp"
#include "../errors/ParseException.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    // Example source code to tokenize
    std::string sourceCode = R"(
        // Example mType program
        namespace math {
            function int add(int a, int b) : int {
                return a + b;
            }
            
            class Calculator {
                int result = 0;
                
                constructor(int initial) {
                    result = initial;
                }
                
                int calculate(int x, int y) : int {
                    result = add(x, y);
                    return result;
                }
            }
        }
        
        function void main() : void {
            using math;
            Calculator calc = new Calculator(10);
            int sum = calc.calculate(5, 3);
            /* Multi-line comment
               showing different tokens */
            string message = "Result: " + sum;
            if (sum >= 8) {
                sum += 2;
            }
        }
    )";

    try {
        // Create lexer instance
        lexer::Lexer lexer(sourceCode, "example.mtype");
        
        std::cout << "=== TOKENIZING SOURCE CODE ===\n\n";
        
        // Tokenize and display all tokens
        token::Token token;
        int tokenCount = 0;
        
        do {
            token = lexer.getNextToken();
            
            std::cout << "Token #" << ++tokenCount << ": ";
            std::cout << "Type=" << static_cast<int>(token.type);
            
            // Display token value based on type
            switch (token.type) {
                case token::TokenType::IDENTIFIER:
                case token::TokenType::STRING_LITERAL:
                    std::cout << ", Value=\"" << token.stringValue << "\"";
                    break;
                case token::TokenType::INT_NUMBER:
                    std::cout << ", Value=" << token.intValue;
                    break;
                case token::TokenType::FLOAT_NUMBER:
                    std::cout << ", Value=" << token.floatValue;
                    break;
                default:
                    if (!token.stringValue.empty()) {
                        std::cout << ", Text=\"" << token.stringValue << "\"";
                    }
                    break;
            }
            
            std::cout << ", Location=" << token.location.toString() << "\n";
            
        } while (token.type != token::TokenType::END);
        
        std::cout << "\n=== TOKENIZATION COMPLETE ===\n";
        std::cout << "Total tokens processed: " << tokenCount << "\n";
        
    } catch (const errors::ParseException& e) {
        std::cerr << "Lexer Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}