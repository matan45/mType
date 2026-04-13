#pragma once
#include <cstddef>

namespace constants
{
    /**
     * Security limits for hardening the interpreter against malicious input
     * (crafted .mt source files or .mtc bytecode).
     *
     * Centralized so all caps are tunable in one place. Values were chosen
     * generously to leave headroom for legitimate generated code while still
     * preventing the unbounded allocation / unbounded recursion classes of
     * vulnerability identified in the security audit (MYT-29).
     */
    namespace security
    {
        // === Bytecode deserialization caps ===
        constexpr size_t MAX_CONSTANT_POOL_ENTRIES   = 1'000'000;
        constexpr size_t MAX_INSTRUCTION_COUNT       = 1'000'000;
        // Raised from 16 to 64 (MYT-62): lambda DEFINE_LAMBDA instructions encode
        // captured variables, parameter types, and metadata as operands. A block
        // lambda with several captures and a typed functional interface can
        // legitimately reach 34+ operands. 64 still bounds allocation tightly.
        constexpr size_t MAX_OPERANDS_PER_INSTR      = 64;
        constexpr size_t MAX_FUNCTION_COUNT          = 100'000;
        constexpr size_t MAX_PARAMETERS_PER_FUNCTION = 256;
        constexpr size_t MAX_EXCEPTION_TABLE_ENTRIES = 100'000;
        constexpr size_t MAX_STRING_BYTES            = 1 << 24; // 16 MiB cap on individual serialized strings

        // === VM runtime caps ===
        constexpr size_t MAX_LOCAL_STACK_PER_FRAME   = 100'000;

        // === Parser / lexer caps ===
        // NOTE: each expression descent burns ~13 RecursionDepthGuard frames
        // (parseTernary + 10 parseBinaryLevel layers + parseUnary + parsePostfix),
        // so a parenthesized sub-expression `(...)` consumes ~13 units of
        // budget per nesting level. 1024 / 13 ≈ 78 paren levels — well above
        // anything legitimate code generators emit, while still catching
        // pathological depth before the C++ call stack runs out (typically
        // ~5000 frames on default thread stacks).
        constexpr size_t MAX_PARSER_RECURSION_DEPTH  = 1024;
        constexpr size_t MAX_OPTIMIZER_DEPTH         = 256;
        constexpr size_t MAX_IDENTIFIER_LENGTH       = 1024;
        constexpr size_t MAX_NUMBER_LITERAL_LENGTH   = 256;
        constexpr size_t MAX_STRING_LITERAL_LENGTH   = 1 << 20;   // 1 MiB

        // === Array dimension caps ===
        constexpr size_t MAX_ARRAY_DIMENSION_PRODUCT = 100'000'000;

        // === .mtcLib library caps ===
        constexpr size_t MAX_MTCLIB_DEPENDENCIES = 1'000;
        constexpr size_t MAX_MTCLIB_EXPORTS      = 100'000;
        constexpr size_t MAX_MTCLIB_IMPORTS       = 100'000;
    }
}
