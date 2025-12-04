#pragma once

namespace token
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
        LBRACKET, RBRACKET, // [ ] (for array literals and indexing)
        COMMA, SEMICOLON, // , ;
        COLON, // : (for return type and for-each)
        QUESTION, // ? (for ternary operator)
        AND, OR, NOT, // &&, ||, !
        BITWISE_AND,    // &
        BITWISE_OR,     // |
        BITWISE_XOR,    // ^
        LEFT_SHIFT,     // <<
        RIGHT_SHIFT,    // >>
        BITWISE_NOT,    // ~ (unary)
        EQUALS, NOT_EQUALS, // ==, !=
        PLUS_ASSIGN, // +=
        MINUS_ASSIGN, // -=
        MULTIPLY_ASSIGN, // *=
        DIVIDE_ASSIGN, // /=
        MODULO_ASSIGN, // %=
        IMPORT, // Add this for import statements
        FROM, // from keyword for selective imports
        LESS, LESS_EQUALS, GREATER, GREATER_EQUALS, // <, <=, >, >=
        INCREMENT, DECREMENT, // ++, --
        FINAL, // Add this for final variables
        ABSTRACT, // abstract keyword for abstract classes and methods
        FUNCTION, RETURN, IF, ELSE, WHILE, FOR, // Keywords
        BREAK, CONTINUE, // Loop control keywords
        INT, FLOAT, BOOL, STRING_TYPE, VOID, // Data types
        TRUE, FALSE, DO,
        CLASS, // class keyword
        NEW, // new keyword
        DOT, // . operator
        STATIC, // static keyword
        PRIVATE, // private keyword
        PUBLIC, // public keyword
        PROTECTED, // protected keyword
        CONSTRUCTOR, // constructor keyword
        SCOPE, // :: scope resolution operator
        SWITCH, CASE, DEFAULT, // Switch-case keywords
        INTERFACE, // interface keyword
        IMPLEMENTS, // implements keyword
        EXTENDS, // extends keyword
        SUPER, // super keyword for parent class access
        AT, // @ for annotations
        ARROW,        // -> for lambda expressions
        ISCLASSOF, // isClassOf keyword for type checking
        ASYNC, // async keyword for async functions
        AWAIT, // await keyword for async expressions
        TRY, // try keyword for exception handling
        CATCH, // catch keyword for exception handling
        THROW, // throw keyword for exception handling
        FINALLY, // finally keyword for exception handling
        END // End of input
    };
}
