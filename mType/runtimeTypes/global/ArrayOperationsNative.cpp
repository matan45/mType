#include "ArrayOperationsNative.hpp"
#include "../../value/operations/ArrayOperations.hpp"
#include "../../value/NativeArray.hpp"
#include "../../errors/RuntimeException.hpp"

namespace runtimeTypes::global
{
    using namespace value;
    using namespace value::operations;

    // Helper methods for argument validation
    void ArrayOperationsNative::validateArgCount(const std::vector<Value>& args, size_t expected, const std::string& funcName)
    {
        if (args.size() != expected) {
            throw errors::RuntimeException(funcName + " requires " + std::to_string(expected) + " argument" + (expected != 1 ? "s" : ""));
        }
    }

    std::shared_ptr<value::NativeArray> ArrayOperationsNative::extractArray(const Value& arg, const std::string& funcName, const std::string& paramName)
    {
        if (!std::holds_alternative<std::shared_ptr<NativeArray>>(arg)) {
            throw errors::RuntimeException(funcName + " requires " + paramName + " to be an array");
        }
        return std::get<std::shared_ptr<NativeArray>>(arg);
    }

    void ArrayOperationsNative::validateTwoArrays(const std::vector<Value>& args, const std::string& funcName)
    {
        validateArgCount(args, 2, funcName);
        if (!std::holds_alternative<std::shared_ptr<NativeArray>>(args[0]) ||
            !std::holds_alternative<std::shared_ptr<NativeArray>>(args[1])) {
            throw errors::RuntimeException(funcName + " requires two arrays");
        }
    }

    void ArrayOperationsNative::validateSingleArray(const std::vector<Value>& args, const std::string& funcName)
    {
        validateArgCount(args, 1, funcName);
        if (!std::holds_alternative<std::shared_ptr<NativeArray>>(args[0])) {
            throw errors::RuntimeException(funcName + " requires an array");
        }
    }

    void ArrayOperationsNative::registerAll(std::shared_ptr<environment::Environment> env)
    {
        auto nativeRegistry = env->getNativeRegistry();

        // Arithmetic operations
        nativeRegistry->registerNativeFunction("arrayAdd", arrayAdd);
        nativeRegistry->registerNativeFunction("arrayAddScalar", arrayAddScalar);
        nativeRegistry->registerNativeFunction("arraySubtract", arraySubtract);
        nativeRegistry->registerNativeFunction("arrayMultiply", arrayMultiply);
        nativeRegistry->registerNativeFunction("arrayMultiplyScalar", arrayMultiplyScalar);

        // Reduction operations
        nativeRegistry->registerNativeFunction("arraySum", arraySum);
        nativeRegistry->registerNativeFunction("arrayMin", arrayMin);
        nativeRegistry->registerNativeFunction("arrayMax", arrayMax);
        nativeRegistry->registerNativeFunction("arrayAverage", arrayAverage);

        // Utility operations
        nativeRegistry->registerNativeFunction("arrayFill", arrayFill);
        nativeRegistry->registerNativeFunction("arrayCopy", arrayCopy);
        nativeRegistry->registerNativeFunction("arrayReverse", arrayReverse);
    }

    // ========== Arithmetic Operations ==========

    Value ArrayOperationsNative::arrayAdd(const std::vector<Value>& args)
    {
        validateTwoArrays(args, "arrayAdd");

        auto array1 = std::get<std::shared_ptr<NativeArray>>(args[0]);
        auto array2 = std::get<std::shared_ptr<NativeArray>>(args[1]);

        return ArrayOperations::add(array1, array2);
    }

    Value ArrayOperationsNative::arrayAddScalar(const std::vector<Value>& args)
    {
        validateArgCount(args, 2, "arrayAddScalar");
        auto array = extractArray(args[0], "arrayAddScalar", "first argument");
        return ArrayOperations::addScalar(array, args[1]);
    }

    Value ArrayOperationsNative::arraySubtract(const std::vector<Value>& args)
    {
        validateTwoArrays(args, "arraySubtract");

        auto array1 = std::get<std::shared_ptr<NativeArray>>(args[0]);
        auto array2 = std::get<std::shared_ptr<NativeArray>>(args[1]);

        return ArrayOperations::subtract(array1, array2);
    }

    Value ArrayOperationsNative::arrayMultiply(const std::vector<Value>& args)
    {
        validateTwoArrays(args, "arrayMultiply");

        auto array1 = std::get<std::shared_ptr<NativeArray>>(args[0]);
        auto array2 = std::get<std::shared_ptr<NativeArray>>(args[1]);

        return ArrayOperations::multiply(array1, array2);
    }

    Value ArrayOperationsNative::arrayMultiplyScalar(const std::vector<Value>& args)
    {
        validateArgCount(args, 2, "arrayMultiplyScalar");
        auto array = extractArray(args[0], "arrayMultiplyScalar", "first argument");
        return ArrayOperations::multiplyScalar(array, args[1]);
    }

    // ========== Reduction Operations ==========

    Value ArrayOperationsNative::arraySum(const std::vector<Value>& args)
    {
        validateSingleArray(args, "arraySum");
        auto array = std::get<std::shared_ptr<NativeArray>>(args[0]);
        return ArrayOperations::sum(array);
    }

    Value ArrayOperationsNative::arrayMin(const std::vector<Value>& args)
    {
        validateSingleArray(args, "arrayMin");
        auto array = std::get<std::shared_ptr<NativeArray>>(args[0]);
        return ArrayOperations::min(array);
    }

    Value ArrayOperationsNative::arrayMax(const std::vector<Value>& args)
    {
        validateSingleArray(args, "arrayMax");
        auto array = std::get<std::shared_ptr<NativeArray>>(args[0]);
        return ArrayOperations::max(array);
    }

    Value ArrayOperationsNative::arrayAverage(const std::vector<Value>& args)
    {
        validateSingleArray(args, "arrayAverage");
        auto array = std::get<std::shared_ptr<NativeArray>>(args[0]);
        return ArrayOperations::average(array);
    }

    // ========== Utility Operations ==========

    Value ArrayOperationsNative::arrayFill(const std::vector<Value>& args)
    {
        validateArgCount(args, 2, "arrayFill");
        auto array = extractArray(args[0], "arrayFill", "first argument");
        ArrayOperations::fill(array, args[1]);
        return std::monostate{}; // void return
    }

    Value ArrayOperationsNative::arrayCopy(const std::vector<Value>& args)
    {
        validateSingleArray(args, "arrayCopy");
        auto array = std::get<std::shared_ptr<NativeArray>>(args[0]);
        return ArrayOperations::copy(array);
    }

    Value ArrayOperationsNative::arrayReverse(const std::vector<Value>& args)
    {
        validateSingleArray(args, "arrayReverse");
        auto array = std::get<std::shared_ptr<NativeArray>>(args[0]);
        ArrayOperations::reverse(array);
        return std::monostate{}; // void return
    }

} // namespace runtimeTypes::global
