#include "ClassParser.hpp"
#include "Parser.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/classes/ConstructorNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../ast/nodes/classes/FieldNode.hpp"
#include "../ast/nodes/classes/NewNode.hpp"

namespace parser
{
    using namespace ast::nodes::classes;
    using namespace token;

    std::unique_ptr<ASTNode> ClassParser::parseClass()
    {
        parser.expectToken(TokenType::CLASS);
        
        if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected class name");
        }
        
        std::string className = parser.getCurrentToken().stringValue;
        parser.advanceToken();
        
        parser.expectToken(TokenType::LBRACE);
        
        auto classNode = std::make_unique<ClassNode>(className);
        
        while (parser.getCurrentToken().type != TokenType::RBRACE && parser.getCurrentToken().type != TokenType::END) {
            if (isMethodOrConstructor()) {
                if (parser.getCurrentToken().type == TokenType::CONSTRUCTOR) {
                    auto constructor = parseConstructor();
                    if (constructor) {
                        classNode->addConstructor(std::move(constructor));
                    }
                } else {
                    auto method = parseMethod();
                    if (method) {
                        classNode->addMethod(std::move(method));
                    }
                }
            } else {
                auto field = parseField();
                if (field) {
                    classNode->addField(std::move(field));
                }
            }
        }
        
        parser.expectToken(TokenType::RBRACE);
        
        return std::move(classNode);
    }

    std::unique_ptr<ASTNode> ClassParser::parseConstructor()
    {
        parser.expectToken(TokenType::CONSTRUCTOR);
        
        parser.expectToken(TokenType::LPAREN);
        
        std::vector<std::pair<std::string, ValueType>> parameters;
        while (parser.getCurrentToken().type != TokenType::RPAREN) {
            if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
                throw std::runtime_error("Expected parameter type");
            }
            
            ValueType paramType = ValueType::UNKNOWN;  
            std::string typeName = parser.getCurrentToken().stringValue;
            if (typeName == "int") paramType = ValueType::INTEGER;
            else if (typeName == "float") paramType = ValueType::FLOAT;
            else if (typeName == "string") paramType = ValueType::STRING;
            else if (typeName == "bool") paramType = ValueType::BOOLEAN;
            
            parser.advanceToken();
            
            if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
                throw std::runtime_error("Expected parameter name");
            }
            
            std::string paramName = parser.getCurrentToken().stringValue;
            parameters.push_back({paramName, paramType});
            parser.advanceToken();
            
            if (parser.getCurrentToken().type == TokenType::COMMA) {
                parser.advanceToken();
            }
        }
        
        parser.expectToken(TokenType::RPAREN);
        
        auto body = parser.parseStatement();
        
        return std::make_unique<ConstructorNode>(std::move(parameters), std::move(body));
    }

    std::unique_ptr<ASTNode> ClassParser::parseMethod()
    {
        ValueType returnType = ValueType::UNKNOWN;
        
        if (parser.getCurrentToken().type == TokenType::IDENTIFIER) {
            std::string typeName = parser.getCurrentToken().stringValue;
            if (typeName == "int") returnType = ValueType::INTEGER;
            else if (typeName == "float") returnType = ValueType::FLOAT;
            else if (typeName == "string") returnType = ValueType::STRING;
            else if (typeName == "bool") returnType = ValueType::BOOLEAN;
            else if (typeName == "void") returnType = ValueType::VOID;
            
            parser.advanceToken();
        }
        
        if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected method name");
        }
        
        std::string methodName = parser.getCurrentToken().stringValue;
        parser.advanceToken();
        
        parser.expectToken(TokenType::LPAREN);
        
        std::vector<std::pair<std::string, ValueType>> parameters;
        while (parser.getCurrentToken().type != TokenType::RPAREN) {
            if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
                throw std::runtime_error("Expected parameter type");
            }
            
            ValueType paramType = ValueType::UNKNOWN;
            std::string typeName = parser.getCurrentToken().stringValue;
            if (typeName == "int") paramType = ValueType::INTEGER;
            else if (typeName == "float") paramType = ValueType::FLOAT;
            else if (typeName == "string") paramType = ValueType::STRING;
            else if (typeName == "bool") paramType = ValueType::BOOLEAN;
            
            parser.advanceToken();
            
            if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
                throw std::runtime_error("Expected parameter name");
            }
            
            std::string paramName = parser.getCurrentToken().stringValue;
            parameters.push_back({paramName, paramType});
            parser.advanceToken();
            
            if (parser.getCurrentToken().type == TokenType::COMMA) {
                parser.advanceToken();
            }
        }
        
        parser.expectToken(TokenType::RPAREN);
        
        auto body = parser.parseStatement();
        
        return std::make_unique<MethodNode>(methodName, returnType, std::move(parameters), 
                                          std::move(body), false, false);
    }

    std::unique_ptr<ASTNode> ClassParser::parseField()
    {
        bool isStatic = false;
        bool isFinal = false;
        
        if (parser.getCurrentToken().type == TokenType::STATIC) {
            isStatic = true;
            parser.advanceToken();
        }
        
        if (parser.getCurrentToken().type == TokenType::FINAL) {
            isFinal = true;
            parser.advanceToken();
        }
        
        ValueType fieldType = ValueType::UNKNOWN;
        if (parser.getCurrentToken().type == TokenType::IDENTIFIER) {
            std::string typeName = parser.getCurrentToken().stringValue;
            if (typeName == "int") fieldType = ValueType::INTEGER;
            else if (typeName == "float") fieldType = ValueType::FLOAT;
            else if (typeName == "string") fieldType = ValueType::STRING;
            else if (typeName == "bool") fieldType = ValueType::BOOLEAN;
            
            parser.advanceToken();
        }
        
        if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected field name");
        }
        
        std::string fieldName = parser.getCurrentToken().stringValue;
        parser.advanceToken();
        
        std::unique_ptr<ASTNode> initialValue = nullptr;
        if (parser.getCurrentToken().type == TokenType::ASSIGN) {
            parser.advanceToken();
            initialValue = parser.parseExpression();
        }
        
        parser.expectToken(TokenType::SEMICOLON);
        
        return std::make_unique<FieldNode>(fieldName, fieldType, std::move(initialValue), 
                                         isStatic, isFinal);
    }

    std::unique_ptr<ASTNode> ClassParser::parseNewExpression()
    {
        parser.expectToken(TokenType::NEW);
        
        if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected class name after 'new'");
        }
        
        std::string className = parser.getCurrentToken().stringValue;
        parser.advanceToken();
        
        parser.expectToken(TokenType::LPAREN);
        
        std::vector<std::unique_ptr<ASTNode>> arguments;
        while (parser.getCurrentToken().type != TokenType::RPAREN) {
            arguments.push_back(parser.parseExpression());
            
            if (parser.getCurrentToken().type == TokenType::COMMA) {
                parser.advanceToken();
            }
        }
        
        parser.expectToken(TokenType::RPAREN);
        
        return std::make_unique<NewNode>(className, std::move(arguments));
    }

    bool ClassParser::isMethodOrConstructor()
    {
        if (parser.getCurrentToken().type == TokenType::CONSTRUCTOR) {
            return true;
        }
        
        size_t currentPos = parser.getCurrentPosition();
        
        if (parser.getCurrentToken().type == TokenType::STATIC || 
            parser.getCurrentToken().type == TokenType::FINAL) {
            parser.advanceToken();
        }
        
        if (parser.getCurrentToken().type == TokenType::IDENTIFIER) {
            parser.advanceToken();
            if (parser.getCurrentToken().type == TokenType::IDENTIFIER) {
                parser.advanceToken();
                if (parser.getCurrentToken().type == TokenType::LPAREN) {
                    parser.setPosition(currentPos);
                    return true;
                }
            }
        }
        
        parser.setPosition(currentPos);
        return false;
    }
}
