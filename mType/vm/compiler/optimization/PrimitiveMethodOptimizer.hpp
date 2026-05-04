#pragma once

#include <string>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include "../../bytecode/OpCode.hpp"

namespace vm::compiler {

/**
 * Phase 3 Optimization: Primitive Method Call Optimizer
 *
 * Recognizes method calls on primitive Box types (Int, Float, Bool, String)
 * and emits specialized bytecode instructions for performance.
 *
 * Example:
 *   Int a = new Int(5);
 *   Int b = new Int(10);
 *   Int c = a.add(b);  // Detected and optimized!
 *
 * Instead of generic CALL_METHOD, emits specialized INVOKE_INT_ADD
 * which unboxes, performs fast arithmetic, and re-boxes with caching.
 */
class PrimitiveMethodOptimizer {
public:
    /**
     * Check if a class is a primitive Box type (Int, Float, Bool, String)
     */
    static bool isPrimitiveBoxType(const std::string& className);

    /**
     * Check if a method on a primitive type can be optimized
     * @param className The Box class name (Int, Float, Bool, String)
     * @param methodName The method name (add, subtract, multiply, etc.)
     * @param argCount Number of arguments
     * @return true if this method call can be optimized
     */
    static bool canOptimizeMethod(const std::string& className,
                                  const std::string& methodName,
                                  size_t argCount);

    /**
     * Get the optimized opcode for a primitive method call
     * @param className The Box class name
     * @param methodName The method name
     * @return The specialized opcode, or OpCode::CALL_METHOD if not optimizable
     */
    static bytecode::OpCode getOptimizedOpCode(const std::string& className,
                                               const std::string& methodName);

    /**
     * Get a description of the optimization for logging/debugging
     */
    static std::string getOptimizationDescription(const std::string& className,
                                                  const std::string& methodName);

private:
    // Primitive Box type names
    static const std::unordered_set<std::string> primitiveBoxTypes_;

    // Map of (className, methodName) -> OpCode for optimizable methods
    // Format: "Int::add" -> INVOKE_INT_ADD
    using MethodKey = std::string;
    static const std::unordered_map<MethodKey, bytecode::OpCode> methodOptimizations_;

    // Helper to create method key
    static std::string makeMethodKey(const std::string& className, const std::string& methodName);
};

} // namespace vm::compiler
