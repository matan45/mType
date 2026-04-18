#pragma once

#include "../context/ExecutionContext.hpp"
#include "../../../value/IntegerCache.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"

namespace vm::runtime {

/**
 * Phase 3: Executor for optimized primitive method calls
 *
 * Handles specialized INVOKE_INT_* and INVOKE_FLOAT_* opcodes.
 *
 * Pattern for each handler:
 * 1. Pop arguments and receiver from stack
 * 2. Unbox: Extract primitive values from object fields
 * 3. Fast operation: Direct arithmetic/comparison
 * 4. Box result: Create result object (with caching for Int)
 * 5. Push result onto stack
 *
 * Performance vs generic CALL_METHOD:
 * - CALL_METHOD: ~50-100 cycles (method lookup, frame setup, call, cleanup)
 * - INVOKE_INT_ADD: ~7-15 cycles (unbox, add, cache lookup, push)
 * - Speedup: 3-7x faster
 */
class PrimitiveMethodExecutor {
public:
    explicit PrimitiveMethodExecutor(ExecutionContext& ctx);
    ~PrimitiveMethodExecutor() = default;

    // === Int Object Method Handlers ===

    // Binary arithmetic operations (receiver.method(arg))
    void handleInvokeIntAdd();      // Int.add(Int) -> Int
    void handleInvokeIntSub();      // Int.subtract(Int) -> Int
    void handleInvokeIntMul();      // Int.multiply(Int) -> Int
    void handleInvokeIntDiv();      // Int.divide(Int) -> Int
    void handleInvokeIntMod();      // Int.modulo(Int) -> Int

    // Unary operations (receiver.method())
    void handleInvokeIntNeg();      // Int.negate() -> Int
    void handleInvokeIntAbs();      // Int.abs() -> Int

    // Comparison operations
    void handleInvokeIntEquals();   // Int.equals(Int) -> bool
    void handleInvokeIntCompare();  // Int.compareTo(Int) -> int

    // Inline accessors (MYT-155): pop boxed receiver, push raw primitive from
    // field 0. Skips the call-frame-setup + interpreter-loop dispatch that
    // CALL_METHOD into Int::getValue/Float::getValue/Bool::getValue would do.
    void handleInvokeIntGetValue();    // Int.getValue() -> int
    void handleInvokeFloatGetValue();  // Float.getValue() -> float
    void handleInvokeBoolGetValue();   // Bool.getValue() -> bool

    // Inline comparison ops (MYT-155): pop arg + receiver, push bool result.
    void handleInvokeIntLessThan();        // Int.lessThan(Int) -> bool
    void handleInvokeIntLessEqual();       // Int.lessThanOrEqual(Int) -> bool
    void handleInvokeIntGreaterThan();     // Int.greaterThan(Int) -> bool
    void handleInvokeIntGreaterEqual();    // Int.greaterThanOrEqual(Int) -> bool
    void handleInvokeFloatLessThan();      // Float.lessThan(Float) -> bool
    void handleInvokeFloatLessEqual();     // Float.lessThanOrEqual(Float) -> bool
    void handleInvokeFloatGreaterThan();   // Float.greaterThan(Float) -> bool
    void handleInvokeFloatGreaterEqual();  // Float.greaterThanOrEqual(Float) -> bool

    // === Float Object Method Handlers ===

    // Binary arithmetic operations
    void handleInvokeFloatAdd();    // Float.add(Float) -> Float
    void handleInvokeFloatSub();    // Float.subtract(Float) -> Float
    void handleInvokeFloatMul();    // Float.multiply(Float) -> Float
    void handleInvokeFloatDiv();    // Float.divide(Float) -> Float

    // Unary operations
    void handleInvokeFloatNeg();    // Float.negate() -> Float
    void handleInvokeFloatAbs();    // Float.abs() -> Float

    // Comparison operations
    void handleInvokeFloatEquals(); // Float.equals(Float) -> bool
    void handleInvokeFloatCompare();// Float.compareTo(Float) -> int

private:
    ExecutionContext& context;

    // === Helper Methods ===

    /**
     * Unbox an Int object to get its primitive int value
     * @param obj The Int object
     * @return The primitive int64_t value from the 'value' field
     */
    int64_t unboxInt(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj);
    double unboxFloat(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj);

    // Value-based unbox: handles both ObjectInstance and ValueObject
    int64_t unboxIntFromValue(const value::Value& val);
    double unboxFloatFromValue(const value::Value& val);

    // Box that returns appropriate type (ValueObject if value class, ObjectInstance otherwise)
    value::Value boxIntValue(int64_t val);
    value::Value boxFloatValue(double val);

    // Legacy box methods (always return ObjectInstance)
    std::shared_ptr<runtimeTypes::klass::ObjectInstance> boxInt(int64_t value);
    std::shared_ptr<runtimeTypes::klass::ObjectInstance> boxFloat(double value);

    /**
     * Get the class definition for Int (cached for performance)
     * @return Shared pointer to Int class definition
     */
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> getIntClass();

    /**
     * Get the class definition for Float (cached for performance)
     * @return Shared pointer to Float class definition
     */
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> getFloatClass();

    // Cached class definitions (lazy initialized)
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> cachedIntClass_;
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> cachedFloatClass_;
};

} // namespace vm::runtime
