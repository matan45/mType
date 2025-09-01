#pragma once

namespace  token
{
    enum class TokenType
    {
        IDENTIFIER,
        FLOAT_NUMBER,
        INT_NUMBER,
        STRING_LITERAL,
        NULL_LITERAL, // null keyword
        PLUS, MINUS, MULTIPLY, DIVIDE, MODULO,
        ASSIGN, // =
        LPAREN, RPAREN, // ( )
        LBRACE, RBRACE, // { }
        COMMA, SEMICOLON, // , ;
        COLON, // : (for return type)
        QUESTION, // ? (for ternary operator)
        AND, OR, NOT, // &&, ||, !
        EQUALS, NOT_EQUALS, // ==, !=
        PLUS_ASSIGN, // +=
        MINUS_ASSIGN, // -=
        MULTIPLY_ASSIGN, // *=
        DIVIDE_ASSIGN, // /=
        MODULO_ASSIGN, // %=
        IMPORT, // Add this for import statements
        LESS, LESS_EQUALS, GREATER, GREATER_EQUALS, // <, <=, >, >=
        INCREMENT, DECREMENT, // ++, --
        FINAL, // Add this for final variables
        FUNCTION, RETURN, IF, ELSE, WHILE, FOR, // Keywords
        BREAK, CONTINUE, // Loop control keywords
        INT, FLOAT, BOOL, STRING_TYPE, VOID, // Data types
        TRUE, FALSE, DO,
        CLASS, // class keyword
        NEW, // new keyword
        DOT, // . operator
        STATIC, // static keyword
        CONSTRUCTOR, // constructor keyword
        NATIVE, // Add this for native function declarations
        SCOPE, // :: scope resolution operator
        SWITCH, CASE, DEFAULT, // Switch-case keywords
        END // End of input
    };
}
