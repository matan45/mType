#pragma once
#include <cstdint>

namespace ast::serialization
{
    // Magic number to identify AST files: "MTAST"
    constexpr uint32_t MAGIC_NUMBER = 0x4D544153;

    // Current serialization format version
    constexpr uint32_t CURRENT_VERSION = 1;

    // Node type identifiers for serialization
    enum class NodeType : uint16_t
    {
        // Expression nodes
        INTEGER_NODE = 100,
        FLOAT_NODE = 101,
        STRING_NODE = 102,
        BOOL_NODE = 103,
        NULL_NODE = 104,
        VARIABLE_NODE = 105,
        BINARY_EXP_NODE = 106,
        UNARY_EXP_NODE = 107,
        TERNARY_EXP_NODE = 108,
        ARRAY_LITERAL_NODE = 109,
        MAP_LITERAL_NODE = 110,
        INDEX_ACCESS_NODE = 111,
        ARRAY_CREATION_NODE = 112,

        // Statement nodes
        PROGRAM_NODE = 200,
        BLOCK_NODE = 201,
        DECLARATION_NODE = 202,
        ASSIGNMENT_NODE = 203,
        MEMBER_ASSIGNMENT_NODE = 204,
        INDEX_ASSIGNMENT_NODE = 205,
        IF_NODE = 206,
        WHILE_NODE = 207,
        DO_WHILE_NODE = 208,
        FOR_NODE = 209,
        FOR_EACH_NODE = 210,
        BREAK_NODE = 211,
        CONTINUE_NODE = 212,
        SWITCH_NODE = 213,
        CASE_NODE = 214,
        DEFAULT_CASE_NODE = 215,
        IMPORT_NODE = 216,
        NATIVE_FUNCTION_NODE = 217,

        // Function nodes
        FUNCTION_NODE = 300,
        FUNCTION_CALL_NODE = 301,
        RETURN_NODE = 302,

        // Class nodes
        CLASS_NODE = 400,
        METHOD_NODE = 401,
        FIELD_NODE = 402,
        CONSTRUCTOR_NODE = 403,
        NEW_NODE = 404,
        MEMBER_ACCESS_NODE = 405,
        METHOD_CALL_NODE = 406
    };

    // Binary operators for serialization (maps TokenType to smaller values)
    enum class BinaryOperator : uint8_t
    {
        ADD = 0,
        SUBTRACT = 1,
        MULTIPLY = 2,
        DIVIDE = 3,
        MODULO = 4,
        EQUAL = 5,
        NOT_EQUAL = 6,
        LESS_THAN = 7,
        LESS_EQUAL = 8,
        GREATER_THAN = 9,
        GREATER_EQUAL = 10,
        LOGICAL_AND = 11,
        LOGICAL_OR = 12,
        ASSIGN = 13,
        PLUS_ASSIGN = 14,
        MINUS_ASSIGN = 15,
        MULTIPLY_ASSIGN = 16,
        DIVIDE_ASSIGN = 17,
        MODULO_ASSIGN = 18
    };

    // Unary operators for serialization
    enum class UnaryOperator : uint8_t
    {
        MINUS = 0,
        NOT = 1,
        PRE_INCREMENT = 2,
        POST_INCREMENT = 3,
        PRE_DECREMENT = 4,
        POST_DECREMENT = 5,
        PLUS = 6
    };

    // Unary position for serialization
    enum class UnaryPosition : uint8_t
    {
        PREFIX = 0,
        POSTFIX = 1
    };

    // Value types for serialization
    enum class ValueType : uint8_t
    {
        INT = 0,
        FLOAT = 1,
        STRING = 2,
        BOOL = 3,
        OBJECT = 4,
        ARRAY = 5,
        MAP = 6,
        SET = 7,
        STACK = 8,
        QUEUE = 9,
        NULL_VALUE = 10,
        VOID = 11
    };

    // File header structure
    struct FileHeader
    {
        uint32_t magic;      // Magic number "MTAST"
        uint32_t version;    // Format version
        uint32_t checksum;   // CRC32 checksum of content
        uint32_t size;       // Size of content in bytes
    };

    // Node header structure
    struct NodeHeader
    {
        NodeType type;       // Node type identifier
        uint32_t line;       // Source line number
        uint32_t column;     // Source column number
        uint32_t dataSize;   // Size of node-specific data in bytes
    };
}