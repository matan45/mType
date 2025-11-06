#include "PrimitiveMethodOptimizer.hpp"

namespace vm::compiler {

// Define primitive Box type names
const std::unordered_set<std::string> PrimitiveMethodOptimizer::primitiveBoxTypes_ = {
    "Int", "Float", "Bool", "String"
};

// Define method optimizations map
const std::unordered_map<std::string, bytecode::OpCode> PrimitiveMethodOptimizer::methodOptimizations_ = {
    // Int methods
    {"Int::add", bytecode::OpCode::INVOKE_INT_ADD},
    {"Int::subtract", bytecode::OpCode::INVOKE_INT_SUB},
    {"Int::multiply", bytecode::OpCode::INVOKE_INT_MUL},
    {"Int::divide", bytecode::OpCode::INVOKE_INT_DIV},
    {"Int::modulo", bytecode::OpCode::INVOKE_INT_MOD},
    {"Int::negate", bytecode::OpCode::INVOKE_INT_NEG},
    {"Int::abs", bytecode::OpCode::INVOKE_INT_ABS},
    {"Int::equals", bytecode::OpCode::INVOKE_INT_EQUALS},
    {"Int::compareTo", bytecode::OpCode::INVOKE_INT_COMPARE},

    // Float methods
    {"Float::add", bytecode::OpCode::INVOKE_FLOAT_ADD},
    {"Float::subtract", bytecode::OpCode::INVOKE_FLOAT_SUB},
    {"Float::multiply", bytecode::OpCode::INVOKE_FLOAT_MUL},
    {"Float::divide", bytecode::OpCode::INVOKE_FLOAT_DIV},
    {"Float::negate", bytecode::OpCode::INVOKE_FLOAT_NEG},
    {"Float::abs", bytecode::OpCode::INVOKE_FLOAT_ABS},
    {"Float::equals", bytecode::OpCode::INVOKE_FLOAT_EQUALS},
    {"Float::compareTo", bytecode::OpCode::INVOKE_FLOAT_COMPARE},
};

bool PrimitiveMethodOptimizer::isPrimitiveBoxType(const std::string& className) {
    // Extract base class name if generic (e.g., "Int<T>" -> "Int")
    size_t anglePos = className.find('<');
    std::string baseClassName = (anglePos != std::string::npos)
        ? className.substr(0, anglePos)
        : className;

    return primitiveBoxTypes_.find(baseClassName) != primitiveBoxTypes_.end();
}

bool PrimitiveMethodOptimizer::canOptimizeMethod(const std::string& className,
                                                const std::string& methodName,
                                                size_t argCount) {
    // Must be a primitive Box type
    if (!isPrimitiveBoxType(className)) {
        return false;
    }

    // Check if this method has an optimization mapping
    std::string key = makeMethodKey(className, methodName);
    auto it = methodOptimizations_.find(key);

    if (it == methodOptimizations_.end()) {
        return false;  // Method not optimizable
    }

    // Validate argument count matches expected
    // Binary operations (add, subtract, multiply, divide, modulo, equals, compareTo): 1 arg
    // Unary operations (negate, abs): 0 args
    if (methodName == "negate" || methodName == "abs") {
        return argCount == 0;
    } else {
        return argCount == 1;
    }
}

bytecode::OpCode PrimitiveMethodOptimizer::getOptimizedOpCode(const std::string& className,
                                                              const std::string& methodName) {
    std::string key = makeMethodKey(className, methodName);
    auto it = methodOptimizations_.find(key);

    if (it != methodOptimizations_.end()) {
        return it->second;
    }

    // Not optimizable - return generic CALL_METHOD
    return bytecode::OpCode::CALL_METHOD;
}

std::string PrimitiveMethodOptimizer::getOptimizationDescription(const std::string& className,
                                                                const std::string& methodName) {
    std::string key = makeMethodKey(className, methodName);
    auto it = methodOptimizations_.find(key);

    if (it != methodOptimizations_.end()) {
        return "Optimized " + className + "." + methodName + "() -> " +
               bytecode::getOpCodeName(it->second);
    }

    return "No optimization available for " + className + "." + methodName + "()";
}

std::string PrimitiveMethodOptimizer::makeMethodKey(const std::string& className,
                                                    const std::string& methodName) {
    // Extract base class name if generic
    size_t anglePos = className.find('<');
    std::string baseClassName = (anglePos != std::string::npos)
        ? className.substr(0, anglePos)
        : className;

    return baseClassName + "::" + methodName;
}

} // namespace vm::compiler
