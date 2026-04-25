#pragma once
#include <cstdint>

namespace vm::bytecode
{
    /**
     * Bytecode instruction set for mType VM
     * Designed for stack-based execution with optimization support
     */
    enum class OpCode : uint8_t {
        // === Stack Operations (0-9) ===
        PUSH_INT,           // Push integer constant (from constant pool or immediate)
        PUSH_FLOAT,         // Push float constant (from constant pool)
        PUSH_STRING,        // Push string constant (from constant pool)
        PUSH_BOOL,          // Push boolean constant (operand: 0/1)
        PUSH_NULL,          // Push null value
        POP,                // Pop top of stack
        DUP,                // Duplicate top of stack
        DUP2,               // Duplicate top 2 stack values
        SWAP,               // Swap top 2 stack values

        // === Arithmetic Operations (10-24) ===
        ADD,                // Generic addition (int/float/string)
        SUB,                // Subtraction
        MUL,                // Multiplication
        DIV,                // Division
        MOD,                // Modulo
        NEG,                // Negate (unary minus)
        INC,                // Increment
        DEC,                // Decrement

        // Type-specialized arithmetic (optimization)
        ADD_INT,            // Integer addition (faster)
        SUB_INT,            // Integer subtraction
        MUL_INT,            // Integer multiplication
        DIV_INT,            // Integer division
        ADD_FLOAT,          // Float addition
        SUB_FLOAT,          // Float subtraction
        MUL_FLOAT,          // Float multiplication
        DIV_FLOAT,          // Float division

        // === Comparison Operations (25-35) ===
        EQ,                 // Equal (==)
        NE,                 // Not equal (!=)
        LT,                 // Less than (<)
        GT,                 // Greater than (>)
        LE,                 // Less than or equal (<=)
        GE,                 // Greater than or equal (>=)

        // Type-specialized comparisons
        EQ_INT,             // Integer equality (faster)
        NE_INT,             // Integer inequality
        LT_INT,             // Integer less than
        GT_INT,             // Integer greater than

        // === Logical Operations (36-39) ===
        AND,                // Logical AND
        OR,                 // Logical OR
        NOT,                // Logical NOT
        XOR,                // Logical XOR

        // === Bitwise Operations ===
        BITWISE_AND_OP,     // Bitwise AND (&)
        BITWISE_OR_OP,      // Bitwise OR (|)
        BITWISE_XOR_OP,     // Bitwise XOR (^)
        LEFT_SHIFT_OP,      // Left shift (<<)
        RIGHT_SHIFT_OP,     // Right shift (>>)
        BITWISE_NOT_OP,     // Bitwise NOT (~) - unary

        // Type-specialized bitwise (INT). Emitted by the compiler when both
        // operands type-infer to INT (mirrors ADD_INT / SUB_INT) and promoted
        // at runtime by trySpecializeBitwise when the generic opcode observes
        // monomorphic-INT operands via type feedback. Handlers skip the tag
        // check and call rawInt() directly.
        BITWISE_AND_INT,
        BITWISE_OR_INT,
        BITWISE_XOR_INT,
        LEFT_SHIFT_INT,
        RIGHT_SHIFT_INT,
        BITWISE_NOT_INT,

        // === Variable Operations (40-49) ===
        LOAD_VAR,           // Load variable (by name from environment)
        STORE_VAR,          // Store to variable (by name)
        DECLARE_VAR,        // Declare new variable
        LOAD_LOCAL,         // Load local variable (by slot index - faster)
        STORE_LOCAL,        // Store to local variable (by slot index)
        // MYT-199: type-quickened LOAD_LOCAL / STORE_LOCAL. Runtime-rewritten
        // from LOAD_LOCAL / STORE_LOCAL once the slot's observed ValueType is
        // monomorphic (N=1). The fast-path executor guards on the tag and
        // skips variant discrimination; on guard failure the opcode is
        // demoted back to the generic form and cachedDeoptCount is bumped
        // sticky so we don't re-promote. RUNTIME-ONLY — same serialization
        // invariant as CALL_METHOD_CACHED.
        LOAD_LOCAL_INT,
        LOAD_LOCAL_FLOAT,
        LOAD_LOCAL_BOOL,
        LOAD_LOCAL_BOXED_INST,
        STORE_LOCAL_INT,
        STORE_LOCAL_FLOAT,
        STORE_LOCAL_BOOL,
        STORE_LOCAL_BOXED_INST,
        LOAD_GLOBAL,        // Load global variable (by name index)
        STORE_GLOBAL,       // Store to global variable (by name index)
        LOAD_UPVALUE,       // Load closure upvalue (for lambdas)
        STORE_UPVALUE,      // Store to closure upvalue
        CLOSE_UPVALUE,      // Close upvalue (move to heap)

        // === Control Flow (50-59) ===
        JUMP,               // Unconditional jump (operand: offset)
        JUMP_IF_FALSE,      // Jump if top of stack is false
        JUMP_IF_TRUE,       // Jump if top of stack is true
        JUMP_IF_NULL,       // Jump if top of stack is null
        JUMP_BACK,          // Jump backward (for loops)
        JUMP_IF_FALSE_OR_POP, // Short-circuit AND optimization
        JUMP_IF_TRUE_OR_POP,  // Short-circuit OR optimization
        RETURN,             // Return from function (no value)
        RETURN_VALUE,       // Return value from function

        // === Function Operations (60-64) ===
        CALL,               // Call function (operand: arg count)
        CALL_FAST,          // Optimized call for known functions
        TAIL_CALL,          // Tail call optimization
        CLOSURE,            // Create closure (operand: function index)

        // === Object/Class Operations (65-79) ===
        NEW_OBJECT,         // Create new object (operand: class index)
        // MYT-134: escape-analysis-promoted allocation. Same operands as
        // NEW_OBJECT (class index, arg count) but pool-borrowed raw
        // ObjectInstance* tracked in CallFrame::stackObjects and released at
        // frame teardown. Serializable (emitted by BytecodeCompiler).
        NEW_STACK,
        GET_FIELD,          // Get object field (operand: field name index)
        SET_FIELD,          // Set object field (operand: field name index)
        GET_FIELD_FAST,     // Get field with cached offset
        SET_FIELD_FAST,     // Set field with cached offset
        INLINE_SET_FIELD,   // Inlined trivial setter (operand: field name index, no push-back)
        GET_STATIC,         // Get static field (operand: class+field index)
        SET_STATIC,         // Set static field (operand: class+field index)
        CALL_METHOD,        // Call instance method (operand: method name + arg count)
        // MYT-173: IC-stable specialization of CALL_METHOD. Embedded target
        // (shape+funcMeta) lives on the Instruction's mutable cachedMethod* fields.
        // INVARIANT: RUNTIME-ONLY. BytecodeCompiler never emits this opcode and
        // BytecodeProgram::readInstructions rejects it on deserialization — a .mtc
        // file containing it is malformed or tampered.
        CALL_METHOD_CACHED,
        // MYT-203: IC-stable polymorphic specialization of CALL_METHOD. Side
        // table holds up to 4 (shape, funcMeta, program, programIndex,
        // qualifiedName, definingClassName) tuples; hot path linear-scans
        // 2-4 shape compares, no icTable hashmap probe. Promoted on MONO→POLY
        // IC transition (independent sticky polyCachedDeoptCount). Demoted to
        // CALL_METHOD on the POLY→MEGA transition (5th shape). Same RUNTIME-
        // ONLY invariant as CALL_METHOD_CACHED.
        CALL_METHOD_POLY_CACHED,
        // MYT-194: IC-stable specialization of GET_FIELD / SET_FIELD. Embedded
        // target (shape + fieldIndex) lives on the Instruction's mutable
        // cachedField* fields. Same RUNTIME-ONLY invariant as CALL_METHOD_CACHED.
        GET_FIELD_CACHED,
        SET_FIELD_CACHED,
        CALL_STATIC,        // Call static method (operand: method name + arg count)
        INVOKE,             // Optimized method call (name + arg count)
        SUPER_INVOKE,       // Super method call
        SUPER_GET_FIELD,    // Get field from parent class
        SUPER_SET_FIELD,    // Set field in parent class
        GET_SUPER,          // Get super class
        SUPER_CONSTRUCTOR,  // Call super constructor
        THIS_CONSTRUCTOR,   // Call another constructor in the same class

        // === Type Operations (80-85) ===
        INSTANCEOF,         // Check if object is instance of class (operand: type name string index)
        INSTANCEOF_TYPEPARAM, // Check instance-of against a type parameter (operand: parameter name string index)
        CAST,               // Cast object to type
        CHECK_TYPE,         // Runtime type check
        TYPE_CONVERT,       // Type conversion
        GET_TYPE,           // Get type of value

        // === Array Operations (85-94) ===
        NEW_ARRAY,          // Create new array (operand: size on stack)
        NEW_ARRAY_MULTI,    // Create multi-dimensional array
        ARRAY_GET,          // Get array element at index
        ARRAY_SET,          // Set array element at index
        ARRAY_GET_INT,      // Type-specialized array get (int elements)
        ARRAY_SET_INT,      // Type-specialized array set (int elements)
        ARRAY_LENGTH,       // Get array length
        ARRAY_LITERAL,      // Create array from literal values (operand: count)

        // SoA Field Access Optimization (avoids object materialization)
        ARRAY_GET_FIELD,    // Get array[index].field (generic, SoA-optimized)
        ARRAY_SET_FIELD,    // Set array[index].field = value (generic, SoA-optimized)

        // Fused local-array operations (eliminates array copy/destroy overhead)
        ARRAY_GET_INT_LOCAL,  // arr from local, pop index, push value (operand: local index)
        ARRAY_SET_INT_LOCAL,  // arr from local, pop index+value (operand: local index)
        ARRAY_LENGTH_LOCAL,   // arr from local, push length (operand: local index)

        // === Lambda Operations (95-99) ===
        LAMBDA,             // Create lambda value
        LAMBDA_INVOKE,      // Invoke lambda
        CAPTURE_VARIABLE,   // Capture variable for closure

        // === Generic Type Operations (100-104) ===
        BIND_GENERIC,       // Bind generic type parameter
        RESOLVE_GENERIC,    // Resolve generic type
        INSTANTIATE_GENERIC,// Instantiate generic class/function

        // === Interface Operations (105-109) ===
        IMPLEMENT_INTERFACE,// Mark class as implementing interface
        CHECK_INTERFACE,    // Check if object implements interface
        LAMBDA_TO_INTERFACE,// Convert lambda to interface (SAM type)

        // === Exception Handling (110-114) ===
        TRY_BEGIN,          // Begin try block
        TRY_END,            // End try block
        CATCH,              // Catch exception
        THROW,              // Throw exception
        FINALLY,            // Finally block

        // === Debugging & Profiling (115-119) ===
        LINE,               // Line number info (operand: line number)
        SOURCE_FILE,        // Source file info (operand: file name index)
        BREAKPOINT,         // Debugger breakpoint
        PROFILE_ENTER,      // Profile function entry
        PROFILE_EXIT,       // Profile function exit

        // === Loop Optimization (120-122) ===
        LOOP_START,         // Mark loop start for optimization
        LOOP_END,           // Mark loop end

        // === Special Operations (123-127) ===
        NOP,                // No operation (for optimization)
        HALT,               // End program execution
        IMPORT,             // Import module

        // === Async/Await Operations (128-130) ===
        CREATE_PROMISE,     // Create Promise from value on stack
        AWAIT,              // Await a Promise value
        PROMISE_RESOLVE,    // Resolve a Promise with value

        // === Phase 3: Primitive Object Method Optimizations (131-150) ===
        // These opcodes optimize method calls on primitive Box types (Int, Float, Bool, String)
        // They unbox, perform fast operations, and re-box results (with caching for Int)

        // Int object methods (131-140)
        INVOKE_INT_ADD,         // Int.add(Int) - optimized addition with caching
        INVOKE_INT_SUB,         // Int.subtract(Int) - optimized subtraction
        INVOKE_INT_MUL,         // Int.multiply(Int) - optimized multiplication
        INVOKE_INT_DIV,         // Int.divide(Int) - optimized division
        INVOKE_INT_MOD,         // Int.modulo(Int) - optimized modulo
        INVOKE_INT_NEG,         // Int.negate() - optimized negation
        INVOKE_INT_ABS,         // Int.abs() - optimized absolute value
        INVOKE_INT_EQUALS,      // Int.equals(Int) - optimized equality
        INVOKE_INT_COMPARE,     // Int.compareTo(Int) - optimized comparison
        INVOKE_INT_GET_VALUE,   // Int.getValue() - inline field-0 read (MYT-155)
        INVOKE_INT_LESS_THAN,    // Int.lessThan(Int) - inline cmp+sete (MYT-155)
        INVOKE_INT_LESS_EQUAL,   // Int.lessThanOrEqual(Int) (MYT-155)
        INVOKE_INT_GREATER_THAN, // Int.greaterThan(Int) (MYT-155)
        INVOKE_INT_GREATER_EQUAL,// Int.greaterThanOrEqual(Int) (MYT-155)

        // Float object methods (141-150)
        INVOKE_FLOAT_ADD,       // Float.add(Float) - optimized addition
        INVOKE_FLOAT_SUB,       // Float.subtract(Float) - optimized subtraction
        INVOKE_FLOAT_MUL,       // Float.multiply(Float) - optimized multiplication
        INVOKE_FLOAT_DIV,       // Float.divide(Float) - optimized division
        INVOKE_FLOAT_NEG,       // Float.negate() - optimized negation
        INVOKE_FLOAT_ABS,       // Float.abs() - optimized absolute value
        INVOKE_FLOAT_EQUALS,    // Float.equals(Float) - optimized equality
        INVOKE_FLOAT_COMPARE,   // Float.compareTo(Float) - optimized comparison
        INVOKE_FLOAT_GET_VALUE, // Float.getValue() - inline field-0 read (MYT-155)
        INVOKE_BOOL_GET_VALUE,  // Bool.getValue() - inline field-0 read (MYT-155)
        INVOKE_FLOAT_LESS_THAN,    // Float.lessThan(Float) (MYT-155)
        INVOKE_FLOAT_LESS_EQUAL,   // Float.lessThanOrEqual(Float) (MYT-155)
        INVOKE_FLOAT_GREATER_THAN, // Float.greaterThan(Float) (MYT-155)
        INVOKE_FLOAT_GREATER_EQUAL,// Float.greaterThanOrEqual(Float) (MYT-155)

        // === Iterator Operations (151-154) ===
        GET_ITERATOR,           // Get iterator from iterable object (calls iterator() method)
        ITERATOR_HAS_NEXT,      // Check if iterator has more elements (calls hasNext())
        ITERATOR_NEXT,          // Get next element from iterator (calls next())
        ITERATOR_CLOSE,         // Close iterator for cleanup (calls close())

        // === Value Type Operations (155-159) ===
        NEW_VALUE_OBJECT,       // Create new value object (operand: class index, arg count)
        OBJECT_TO_VALUE,        // Convert ObjectInstance on stack top to ValueObject

        // === String Operations (160) ===
        STRING_BUILD,           // Build string from N segments on stack (operand: count)

        // === Reserved for Future Use (161-255) ===
        // Opcodes reserved for future extensions
        INLINE_GET_FIELD,   // Inlined trivial getter (operand: field name index, pushes field value)

        // === Superinstruction Fusion (MYT-198) ===
        // Runtime-fused adjacent pairs. First instr of the pair becomes NOP; this
        // opcode occupies the second slot and does the combined work. fusedSlot on
        // the Instruction carries the captured LOAD_LOCAL / PUSH_INT operand.
        ADD_INT_CONST,            // PUSH_INT k + ADD_INT → int literal from operand[0] + tos (operand[0] = int literal)
        LOAD_LOCAL_CALL_CACHED,   // LOAD_LOCAL s + CALL_METHOD_CACHED → shape-guard, direct dispatch (fusedSlot = s; operands + cached* reused from CALL_METHOD_CACHED)
        LOAD_LOCAL_CALL_POLY_CACHED, // MYT-203: LOAD_LOCAL s + CALL_METHOD_POLY_CACHED → 2-4 shape-guards, direct dispatch (fusedSlot = s; operands + poly* reused from CALL_METHOD_POLY_CACHED)
        LOAD_LOCAL_GET_FIELD_CACHED, // LOAD_LOCAL s + GET_FIELD_CACHED → shape-guard, indexed field read (fusedSlot = s; operands + cached* reused from GET_FIELD_CACHED)

        // === Superinstruction Fusion (MYT-202, compile-time, SERIALIZABLE) ===
        // Emitted by the compile-time peephole pass. Unlike MYT-198, these are
        // NOT runtime-only — they round-trip through .mtc and are accepted by
        // the deserializer. Operands live in Instruction::operands directly; no
        // CachedInstructionState side-table use. Shrink the instruction vector
        // by (K-1) on fusion of K original ops, actually halving/thirding the
        // interpreter dispatch count for the fused sequence.
        LOAD_LOAD_ADD_INT,        // LOAD_LOCAL s1 + LOAD_LOCAL s2 + ADD_INT   (operands: [s1, s2])
        LOAD_LOAD_SUB_INT,        // LOAD_LOCAL s1 + LOAD_LOCAL s2 + SUB_INT   (operands: [s1, s2])
        LOAD_LOAD_MUL_INT,        // LOAD_LOCAL s1 + LOAD_LOCAL s2 + MUL_INT   (operands: [s1, s2])
        LOAD_GET_FIELD,           // LOAD_LOCAL s  + GET_FIELD name_idx        (operands: [s, name_idx])
        LOAD_STORE_LOCAL,         // LOAD_LOCAL src + STORE_LOCAL dst          (operands: [src, dst])
        ADD_INT_STORE_LOCAL,      // ADD_INT + STORE_LOCAL dst                 (operands: [dst])

        // Sentinel — must remain the last entry. Used by isValidOpCode and
        // bytecode deserialization to range-check incoming opcode bytes
        // without requiring manual updates each time a new opcode is added.
        // NOT a valid opcode itself.
        OPCODE_SENTINEL_,
    };

    /**
     * Get human-readable name for opcode (for disassembly and debugging)
     */
    inline const char* getOpCodeName(OpCode opcode) {
        switch (opcode) {
            case OpCode::PUSH_INT: return "PUSH_INT";
            case OpCode::PUSH_FLOAT: return "PUSH_FLOAT";
            case OpCode::PUSH_STRING: return "PUSH_STRING";
            case OpCode::PUSH_BOOL: return "PUSH_BOOL";
            case OpCode::PUSH_NULL: return "PUSH_NULL";
            case OpCode::POP: return "POP";
            case OpCode::DUP: return "DUP";
            case OpCode::DUP2: return "DUP2";
            case OpCode::SWAP: return "SWAP";

            case OpCode::ADD: return "ADD";
            case OpCode::SUB: return "SUB";
            case OpCode::MUL: return "MUL";
            case OpCode::DIV: return "DIV";
            case OpCode::MOD: return "MOD";
            case OpCode::NEG: return "NEG";
            case OpCode::INC: return "INC";
            case OpCode::DEC: return "DEC";
            case OpCode::ADD_INT: return "ADD_INT";
            case OpCode::SUB_INT: return "SUB_INT";
            case OpCode::MUL_INT: return "MUL_INT";
            case OpCode::DIV_INT: return "DIV_INT";
            case OpCode::ADD_FLOAT: return "ADD_FLOAT";
            case OpCode::SUB_FLOAT: return "SUB_FLOAT";
            case OpCode::MUL_FLOAT: return "MUL_FLOAT";
            case OpCode::DIV_FLOAT: return "DIV_FLOAT";

            case OpCode::EQ: return "EQ";
            case OpCode::NE: return "NE";
            case OpCode::LT: return "LT";
            case OpCode::GT: return "GT";
            case OpCode::LE: return "LE";
            case OpCode::GE: return "GE";
            case OpCode::EQ_INT: return "EQ_INT";
            case OpCode::NE_INT: return "NE_INT";
            case OpCode::LT_INT: return "LT_INT";
            case OpCode::GT_INT: return "GT_INT";

            case OpCode::AND: return "AND";
            case OpCode::OR: return "OR";
            case OpCode::NOT: return "NOT";
            case OpCode::XOR: return "XOR";

            case OpCode::BITWISE_AND_OP: return "BITWISE_AND_OP";
            case OpCode::BITWISE_OR_OP: return "BITWISE_OR_OP";
            case OpCode::BITWISE_XOR_OP: return "BITWISE_XOR_OP";
            case OpCode::LEFT_SHIFT_OP: return "LEFT_SHIFT_OP";
            case OpCode::RIGHT_SHIFT_OP: return "RIGHT_SHIFT_OP";
            case OpCode::BITWISE_NOT_OP: return "BITWISE_NOT_OP";
            case OpCode::BITWISE_AND_INT: return "BITWISE_AND_INT";
            case OpCode::BITWISE_OR_INT: return "BITWISE_OR_INT";
            case OpCode::BITWISE_XOR_INT: return "BITWISE_XOR_INT";
            case OpCode::LEFT_SHIFT_INT: return "LEFT_SHIFT_INT";
            case OpCode::RIGHT_SHIFT_INT: return "RIGHT_SHIFT_INT";
            case OpCode::BITWISE_NOT_INT: return "BITWISE_NOT_INT";

            case OpCode::LOAD_VAR: return "LOAD_VAR";
            case OpCode::STORE_VAR: return "STORE_VAR";
            case OpCode::DECLARE_VAR: return "DECLARE_VAR";
            case OpCode::LOAD_LOCAL: return "LOAD_LOCAL";
            case OpCode::STORE_LOCAL: return "STORE_LOCAL";
            case OpCode::LOAD_LOCAL_INT: return "LOAD_LOCAL_INT";
            case OpCode::LOAD_LOCAL_FLOAT: return "LOAD_LOCAL_FLOAT";
            case OpCode::LOAD_LOCAL_BOOL: return "LOAD_LOCAL_BOOL";
            case OpCode::LOAD_LOCAL_BOXED_INST: return "LOAD_LOCAL_BOXED_INST";
            case OpCode::STORE_LOCAL_INT: return "STORE_LOCAL_INT";
            case OpCode::STORE_LOCAL_FLOAT: return "STORE_LOCAL_FLOAT";
            case OpCode::STORE_LOCAL_BOOL: return "STORE_LOCAL_BOOL";
            case OpCode::STORE_LOCAL_BOXED_INST: return "STORE_LOCAL_BOXED_INST";
            case OpCode::LOAD_GLOBAL: return "LOAD_GLOBAL";
            case OpCode::STORE_GLOBAL: return "STORE_GLOBAL";
            case OpCode::LOAD_UPVALUE: return "LOAD_UPVALUE";
            case OpCode::STORE_UPVALUE: return "STORE_UPVALUE";
            case OpCode::CLOSE_UPVALUE: return "CLOSE_UPVALUE";

            case OpCode::JUMP: return "JUMP";
            case OpCode::JUMP_IF_FALSE: return "JUMP_IF_FALSE";
            case OpCode::JUMP_IF_TRUE: return "JUMP_IF_TRUE";
            case OpCode::JUMP_IF_NULL: return "JUMP_IF_NULL";
            case OpCode::JUMP_BACK: return "JUMP_BACK";
            case OpCode::JUMP_IF_FALSE_OR_POP: return "JUMP_IF_FALSE_OR_POP";
            case OpCode::JUMP_IF_TRUE_OR_POP: return "JUMP_IF_TRUE_OR_POP";
            case OpCode::RETURN: return "RETURN";
            case OpCode::RETURN_VALUE: return "RETURN_VALUE";

            case OpCode::CALL: return "CALL";
            case OpCode::CALL_FAST: return "CALL_FAST";
            case OpCode::TAIL_CALL: return "TAIL_CALL";
            case OpCode::CLOSURE: return "CLOSURE";

            case OpCode::NEW_OBJECT: return "NEW_OBJECT";
            case OpCode::NEW_STACK: return "NEW_STACK";
            case OpCode::GET_FIELD: return "GET_FIELD";
            case OpCode::SET_FIELD: return "SET_FIELD";
            case OpCode::GET_FIELD_FAST: return "GET_FIELD_FAST";
            case OpCode::SET_FIELD_FAST: return "SET_FIELD_FAST";
            case OpCode::INLINE_SET_FIELD: return "INLINE_SET_FIELD";
            case OpCode::INLINE_GET_FIELD: return "INLINE_GET_FIELD";
            case OpCode::GET_STATIC: return "GET_STATIC";
            case OpCode::SET_STATIC: return "SET_STATIC";
            case OpCode::CALL_METHOD: return "CALL_METHOD";
            case OpCode::CALL_METHOD_CACHED: return "CALL_METHOD_CACHED";
            case OpCode::CALL_METHOD_POLY_CACHED: return "CALL_METHOD_POLY_CACHED";
            case OpCode::GET_FIELD_CACHED: return "GET_FIELD_CACHED";
            case OpCode::SET_FIELD_CACHED: return "SET_FIELD_CACHED";
            case OpCode::CALL_STATIC: return "CALL_STATIC";
            case OpCode::INVOKE: return "INVOKE";
            case OpCode::SUPER_INVOKE: return "SUPER_INVOKE";
            case OpCode::SUPER_GET_FIELD: return "SUPER_GET_FIELD";
            case OpCode::SUPER_SET_FIELD: return "SUPER_SET_FIELD";
            case OpCode::GET_SUPER: return "GET_SUPER";
            case OpCode::SUPER_CONSTRUCTOR: return "SUPER_CONSTRUCTOR";
            case OpCode::THIS_CONSTRUCTOR: return "THIS_CONSTRUCTOR";

            case OpCode::INSTANCEOF: return "INSTANCEOF";
            case OpCode::INSTANCEOF_TYPEPARAM: return "INSTANCEOF_TYPEPARAM";
            case OpCode::CAST: return "CAST";
            case OpCode::CHECK_TYPE: return "CHECK_TYPE";
            case OpCode::TYPE_CONVERT: return "TYPE_CONVERT";
            case OpCode::GET_TYPE: return "GET_TYPE";

            case OpCode::NEW_ARRAY: return "NEW_ARRAY";
            case OpCode::NEW_ARRAY_MULTI: return "NEW_ARRAY_MULTI";
            case OpCode::ARRAY_GET: return "ARRAY_GET";
            case OpCode::ARRAY_SET: return "ARRAY_SET";
            case OpCode::ARRAY_GET_INT: return "ARRAY_GET_INT";
            case OpCode::ARRAY_SET_INT: return "ARRAY_SET_INT";
            case OpCode::ARRAY_LENGTH: return "ARRAY_LENGTH";
            case OpCode::ARRAY_LITERAL: return "ARRAY_LITERAL";
            case OpCode::ARRAY_GET_FIELD: return "ARRAY_GET_FIELD";
            case OpCode::ARRAY_SET_FIELD: return "ARRAY_SET_FIELD";
            case OpCode::ARRAY_GET_INT_LOCAL: return "ARRAY_GET_INT_LOCAL";
            case OpCode::ARRAY_SET_INT_LOCAL: return "ARRAY_SET_INT_LOCAL";
            case OpCode::ARRAY_LENGTH_LOCAL: return "ARRAY_LENGTH_LOCAL";

            case OpCode::LAMBDA: return "LAMBDA";
            case OpCode::LAMBDA_INVOKE: return "LAMBDA_INVOKE";
            case OpCode::CAPTURE_VARIABLE: return "CAPTURE_VARIABLE";

            case OpCode::BIND_GENERIC: return "BIND_GENERIC";
            case OpCode::RESOLVE_GENERIC: return "RESOLVE_GENERIC";
            case OpCode::INSTANTIATE_GENERIC: return "INSTANTIATE_GENERIC";

            case OpCode::IMPLEMENT_INTERFACE: return "IMPLEMENT_INTERFACE";
            case OpCode::CHECK_INTERFACE: return "CHECK_INTERFACE";
            case OpCode::LAMBDA_TO_INTERFACE: return "LAMBDA_TO_INTERFACE";

            case OpCode::TRY_BEGIN: return "TRY_BEGIN";
            case OpCode::TRY_END: return "TRY_END";
            case OpCode::CATCH: return "CATCH";
            case OpCode::THROW: return "THROW";
            case OpCode::FINALLY: return "FINALLY";

            case OpCode::LINE: return "LINE";
            case OpCode::SOURCE_FILE: return "SOURCE_FILE";
            case OpCode::BREAKPOINT: return "BREAKPOINT";
            case OpCode::PROFILE_ENTER: return "PROFILE_ENTER";
            case OpCode::PROFILE_EXIT: return "PROFILE_EXIT";

            case OpCode::LOOP_START: return "LOOP_START";
            case OpCode::LOOP_END: return "LOOP_END";

            case OpCode::NOP: return "NOP";
            case OpCode::HALT: return "HALT";
            case OpCode::IMPORT: return "IMPORT";

            case OpCode::CREATE_PROMISE: return "CREATE_PROMISE";
            case OpCode::AWAIT: return "AWAIT";
            case OpCode::PROMISE_RESOLVE: return "PROMISE_RESOLVE";

            case OpCode::INVOKE_INT_ADD: return "INVOKE_INT_ADD";
            case OpCode::INVOKE_INT_SUB: return "INVOKE_INT_SUB";
            case OpCode::INVOKE_INT_MUL: return "INVOKE_INT_MUL";
            case OpCode::INVOKE_INT_DIV: return "INVOKE_INT_DIV";
            case OpCode::INVOKE_INT_MOD: return "INVOKE_INT_MOD";
            case OpCode::INVOKE_INT_NEG: return "INVOKE_INT_NEG";
            case OpCode::INVOKE_INT_ABS: return "INVOKE_INT_ABS";
            case OpCode::INVOKE_INT_EQUALS: return "INVOKE_INT_EQUALS";
            case OpCode::INVOKE_INT_COMPARE: return "INVOKE_INT_COMPARE";
            case OpCode::INVOKE_INT_GET_VALUE: return "INVOKE_INT_GET_VALUE";
            case OpCode::INVOKE_INT_LESS_THAN: return "INVOKE_INT_LESS_THAN";
            case OpCode::INVOKE_INT_LESS_EQUAL: return "INVOKE_INT_LESS_EQUAL";
            case OpCode::INVOKE_INT_GREATER_THAN: return "INVOKE_INT_GREATER_THAN";
            case OpCode::INVOKE_INT_GREATER_EQUAL: return "INVOKE_INT_GREATER_EQUAL";

            case OpCode::INVOKE_FLOAT_ADD: return "INVOKE_FLOAT_ADD";
            case OpCode::INVOKE_FLOAT_SUB: return "INVOKE_FLOAT_SUB";
            case OpCode::INVOKE_FLOAT_MUL: return "INVOKE_FLOAT_MUL";
            case OpCode::INVOKE_FLOAT_DIV: return "INVOKE_FLOAT_DIV";
            case OpCode::INVOKE_FLOAT_NEG: return "INVOKE_FLOAT_NEG";
            case OpCode::INVOKE_FLOAT_ABS: return "INVOKE_FLOAT_ABS";
            case OpCode::INVOKE_FLOAT_EQUALS: return "INVOKE_FLOAT_EQUALS";
            case OpCode::INVOKE_FLOAT_COMPARE: return "INVOKE_FLOAT_COMPARE";
            case OpCode::INVOKE_FLOAT_GET_VALUE: return "INVOKE_FLOAT_GET_VALUE";
            case OpCode::INVOKE_BOOL_GET_VALUE: return "INVOKE_BOOL_GET_VALUE";
            case OpCode::INVOKE_FLOAT_LESS_THAN: return "INVOKE_FLOAT_LESS_THAN";
            case OpCode::INVOKE_FLOAT_LESS_EQUAL: return "INVOKE_FLOAT_LESS_EQUAL";
            case OpCode::INVOKE_FLOAT_GREATER_THAN: return "INVOKE_FLOAT_GREATER_THAN";
            case OpCode::INVOKE_FLOAT_GREATER_EQUAL: return "INVOKE_FLOAT_GREATER_EQUAL";

            case OpCode::GET_ITERATOR: return "GET_ITERATOR";
            case OpCode::ITERATOR_HAS_NEXT: return "ITERATOR_HAS_NEXT";
            case OpCode::ITERATOR_NEXT: return "ITERATOR_NEXT";
            case OpCode::ITERATOR_CLOSE: return "ITERATOR_CLOSE";

            case OpCode::NEW_VALUE_OBJECT: return "NEW_VALUE_OBJECT";
            case OpCode::OBJECT_TO_VALUE: return "OBJECT_TO_VALUE";

            case OpCode::STRING_BUILD: return "STRING_BUILD";

            case OpCode::ADD_INT_CONST: return "ADD_INT_CONST";
            case OpCode::LOAD_LOCAL_CALL_CACHED: return "LOAD_LOCAL_CALL_CACHED";
            case OpCode::LOAD_LOCAL_CALL_POLY_CACHED: return "LOAD_LOCAL_CALL_POLY_CACHED";
            case OpCode::LOAD_LOCAL_GET_FIELD_CACHED: return "LOAD_LOCAL_GET_FIELD_CACHED";

            case OpCode::LOAD_LOAD_ADD_INT: return "LOAD_LOAD_ADD_INT";
            case OpCode::LOAD_LOAD_SUB_INT: return "LOAD_LOAD_SUB_INT";
            case OpCode::LOAD_LOAD_MUL_INT: return "LOAD_LOAD_MUL_INT";
            case OpCode::LOAD_GET_FIELD: return "LOAD_GET_FIELD";
            case OpCode::LOAD_STORE_LOCAL: return "LOAD_STORE_LOCAL";
            case OpCode::ADD_INT_STORE_LOCAL: return "ADD_INT_STORE_LOCAL";

            default: return "UNKNOWN";
        }
    }

    /**
     * Number of legal opcodes. Derived from the OPCODE_SENTINEL_ marker so
     * adding a new opcode before the sentinel automatically extends the
     * valid range — no manual update needed here.
     */
    static_assert(static_cast<size_t>(OpCode::OPCODE_SENTINEL_) <= 256,
                  "OpCode enum exceeds uint8_t range; widen the wire format "
                  "before adding more opcodes");
    constexpr uint8_t OPCODE_COUNT = static_cast<uint8_t>(OpCode::OPCODE_SENTINEL_);

    /**
     * Range-check a deserialized opcode byte against the legal enum range.
     * Used by BytecodeProgram::readInstructions() to reject malformed .mtc
     * files that contain reserved or out-of-range opcode bytes.
     * Strict `<` because OPCODE_SENTINEL_ is not itself a valid opcode.
     */
    inline bool isValidOpCode(OpCode opcode) {
        return static_cast<uint8_t>(opcode) < OPCODE_COUNT;
    }
}

