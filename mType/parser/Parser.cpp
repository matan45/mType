#include "Parser.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/expressions/StringNode.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/statements/ImportNode.hpp"
#include "../ast/nodes/statements/ImportedDeclarationNode.hpp"
#include "../ast/nodes/functions/FunctionNode.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/namespaces/NamespaceNode.hpp"
#include <iostream>

namespace parser
{
    using namespace ast::nodes::statements;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::functions;
    using namespace token;

    Parser::Parser(Lexer& lex) 
        : lexer(lex), currentToken(lexer.getNextToken()), importManager(nullptr)
    {
        // Initialize subparsers with reference to main parser
        statementParser = std::make_unique<StatementParser>(*this);
        expressionParser = std::make_unique<ExpressionParser>(*this);
        namespaceParser = std::make_unique<NamespaceParser>(*this);
        classParser = std::make_unique<ClassParser>(*this);
    }

    std::unique_ptr<ASTNode> Parser::parseProgram()
    {
        auto program = std::make_unique<ProgramNode>(currentToken.location);
        
        while (currentToken.type != TokenType::END)
        {
            try {
                auto statement = parseStatement();
                if (statement) {
                    // Check if this is an import statement
                    if (auto importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(statement.get())) {
                        // Handle import by inlining the imported declarations
                        handleImportStatement(importNode, program.get());
                        // Note: We don't add the import node itself to the program
                    } else {
                        program->addStatement(std::move(statement));
                    }
                }
            }
            catch (const std::exception& e) {
                // Handle parse errors - report error and rethrow for naming validation
                if (std::string(e.what()).find("must start with") != std::string::npos) {
                    // This is our naming validation error - rethrow it
                    throw;
                }
                // For other parse errors, log and continue
                std::cerr << "Parse error: " << e.what() << std::endl;
                advance(); // Skip problematic token and continue
            }
        }
        
        return std::move(program);
    }

    void Parser::advance()
    {
        currentToken = lexer.getNextToken();
    }

    bool Parser::match(TokenType type)
    {
        if (currentToken.type == type) {
            advance();
            return true;
        }
        return false;
    }

    void Parser::expect(TokenType type)
    {
        if (!match(type)) {
            throw std::runtime_error("Expected token type " + std::to_string(static_cast<int>(type)));
        }
    }

    std::unique_ptr<ASTNode> Parser::parseStatement()
    {
        // Delegate to StatementParser
        return statementParser->parseStatement();
    }

    std::unique_ptr<ASTNode> Parser::parseExpression()
    {
        // Delegate to ExpressionParser
        return expressionParser->parseExpression();
    }

    void Parser::handleImportStatement(ast::nodes::statements::ImportNode* importNode, 
                                     ast::nodes::statements::ProgramNode* program)
    {
        if (!importManager) {
            throw std::runtime_error("ImportManager not set - cannot process import statement");
        }
        
        if (!importNode->isResolved()) {
            throw std::runtime_error("Import not resolved: " + importNode->getFilePath());
        }
        
        // Get the imported AST
        ASTNode* importedAST = importNode->getImportedAST();
        if (!importedAST) {
            return; // Nothing to import
        }
        
        // INLINE APPROACH: Extract declarations from imported AST and add them directly
        // Cast to ProgramNode (imported files are parsed as programs)
        if (auto importedProgram = dynamic_cast<ProgramNode*>(importedAST)) {
            
            std::cout << "Processing import: " << importNode->getFilePath() << std::endl;
            
            // Get all statements from the imported program
            const auto& importedStatements = importedProgram->getStatements();
            
            // Extract and inline importable declarations
            for (const auto& stmt : importedStatements) {
                if (isImportableDeclaration(stmt.get())) {
                    // Since we can't easily clone AST nodes without implementing clone methods,
                    // we'll use a practical approach: create ImportedDeclarationNode
                    // that wraps the original declaration and preserves the import context
                    
                    auto importedDecl = std::make_unique<ImportedDeclarationNode>(
                        stmt.get(),  // Reference to original declaration (ImportManager keeps it alive)
                        importNode->getFilePath(),  // Source file for debugging
                        stmt->getLocation()  // Original location
                    );
                    
                    program->addStatement(std::move(importedDecl));
                }
            }
            
            std::cout << "Imported " << importedStatements.size() << " statements from " 
                      << importNode->getFilePath() << std::endl;
        }
    }

    bool Parser::isImportableDeclaration(ASTNode* node)
    {
        // Import these types of declarations:
        // - Function definitions (including native functions)
        // - Global variable declarations (assignments at top level)
        // - Class definitions
        // - Namespace definitions
        // 
        // Don't import:
        // - Import statements (to avoid recursive imports)
        // - Other statement types that aren't declarations
        
        // Skip import statements to avoid recursion
        if (dynamic_cast<ImportNode*>(node) != nullptr) {
            return false;
        }
        
        return dynamic_cast<ast::nodes::functions::FunctionNode*>(node) != nullptr ||
               dynamic_cast<ast::nodes::classes::ClassNode*>(node) != nullptr ||
               dynamic_cast<ast::nodes::namespaces::NamespaceNode*>(node) != nullptr ||
               dynamic_cast<AssignmentNode*>(node) != nullptr; // Global variables
    }

}
