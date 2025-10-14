#include "ArrayOperationsNative.hpp"
#include "../../value/operations/ArrayOperations.hpp"
#include "../../value/NativeArray.hpp"
#include "../../errors/RuntimeException.hpp"

namespace runtimeTypes::global
{
    using namespace value;
    using namespace value::operations;

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
        if (args.size() != 2) {
            throw errors::RuntimeException("arrayAdd requires 2 arguments");
        }

        if (!std::holds_alternative<std::shared_ptr<NativeArray>>(args[0]) ||
            !std::holds_alternative<std::shared_ptr<NativeArray>>(args[1])) {
            throw errors::RuntimeException("arrayAdd requires two arrays");
        }

        auto array1 = std::get<std::shared_ptr<NativeArray>>(args[0]);
        auto array2 = std::get<std::shared_ptr<NativeArray>>(args[1]);

        return ArrayOperations::add(array1, array2);
    }

    Value ArrayOperationsNative::arrayAddScalar(const std::vector<Value>& args)
    {
        if (args.size() != 2) {
            throw errors::RuntimeException("arrayAddScalar requires 2 arguments");
        }

        if (!std::holds_alternative<std::shared_ptr<NativeArray>>(args[0])) {
            throw errors::RuntimeException("arrayAddScalar requires an array as first argument");
        }

        auto array = std::get<std::shared_ptr<NativeArray>>(args[0]);
        return ArrayOperations::addScalar(array, args[1]);
    }

    Value ArrayOperationsNative::arraySubtract(const std::vector<Value>& args)
    {
        if (args.size() != 2) {
            throw errors::RuntimeException("arraySubtract requires 2 arguments");
        }

        if (!std::holds_alternative<std::shared_ptr<NativeArray>>(args[0]) ||
            !std::holds_alternative<std::shared_ptr<NativeArray>>(args[1])) {
            throw errors::RuntimeException("arraySubtract requires two arrays");
        }

        auto array1 = std::get<std::shared_ptr<NativeArray>>(args[0]);
        auto array2 = std::get<std::shared_ptr<NativeArray>>(args[1]);

        return ArrayOperations::subtract(array1, array2);
    }

    Value ArrayOperationsNative::arrayMultiply(const std::vector<Value>& args)
    {
        if (args.size() != 2) {
            throw errors::RuntimeException("arrayMultiply requires 2 arguments");
        }

        if (!std::holds_alternative<std::shared_ptr<NativeArray>>(args[0]) ||
            !std::holds_alternative<std::shared_ptr<NativeArray>>(args[1])) {
            throw errors::RuntimeException("arrayMultiply requires two arrays");
        }

        auto array1 = std::get<std::shared_ptr<NativeArray>>(args[0]);
        auto array2 = std::get<std::shared_ptr<NativeArray>>(args[1]);

        return ArrayOperations::multiply(array1, array2);
    }

    Value ArrayOperationsNative::arrayMultiplyScalar(const std::vector<Value>& args)
    {
        if (args.size() != 2) {
            throw errors::RuntimeException("arrayMultiplyScalar requires 2 arguments");
        }

        if (!std::holds_alternative<std::shared_ptr<NativeArray>>(args[0])) {
            throw errors::RuntimeException("arrayMultiplyScalar requires an array as first argument");
        }

        auto array = std::get<std::shared_ptr<NativeArray>>(args[0]);
        return ArrayOperations::multiplyScalar(array, args[1]);
    }

    // ========== Reduction Operations ==========

    Value ArrayOperationsNative::arraySum(const std::vector<Value>& args)
    {
        if (args.size() != 1) {
            throw errors::RuntimeException("arraySum requires 1 argument");
        }

        if (!std::holds_alternative<std::shared_ptr<NativeArray>>(args[0])) {
            throw errors::RuntimeException("arraySum requires an array");
        }

        auto array = std::get<std::shared_ptr<NativeArray>>(args[0]);
        return ArrayOperations::sum(array);
    }

    Value ArrayOperationsNative::arrayMin(const std::vector<Value>& args)
    {
        if (args.size() != 1) {
            throw errors::RuntimeException("arrayMin requires 1 argument");
        }

        if (!std::holds_alternative<std::shared_ptr<NativeArray>>(args[0])) {
            throw errors::RuntimeException("arrayMin requires an array");
        }

        auto array = std::get<std::shared_ptr<NativeArray>>(args[0]);
        return ArrayOperations::min(array);
    }

    Value ArrayOperationsNative::arrayMax(const std::vector<Value>& args)
    {
        if (args.size() != 1) {
            throw errors::RuntimeException("arrayMax requires 1 argument");
        }

        if (!std::holds_alternative<std::shared_ptr<NativeArray>>(args[0])) {
            throw errors::RuntimeException("arrayMax requires an array");
        }

        auto array = std::get<std::shared_ptr<NativeArray>>(args[0]);
        return ArrayOperations::max(array);
    }

    Value ArrayOperationsNative::arrayAverage(const std::vector<Value>& args)
    {
        if (args.size() != 1) {
            throw errors::RuntimeException("arrayAverage requires 1 argument");
        }

        if (!std::holds_alternative<std::shared_ptr<NativeArray>>(args[0])) {
            throw errors::RuntimeException("arrayAverage requires an array");
        }

        auto array = std::get<std::shared_ptr<NativeArray>>(args[0]);
        return ArrayOperations::average(array);
    }

    // ========== Utility Operations ==========

    Value ArrayOperationsNative::arrayFill(const std::vector<Value>& args)
    {
        if (args.size() != 2) {
            throw errors::RuntimeException("arrayFill requires 2 arguments");
        }

        if (!std::holds_alternative<std::shared_ptr<NativeArray>>(args[0])) {
            throw errors::RuntimeException("arrayFill requires an array as first argument");
        }

        auto array = std::get<std::shared_ptr<NativeArray>>(args[0]);
        ArrayOperations::fill(array, args[1]);
        return std::monostate{}; // void return
    }

    Value ArrayOperationsNative::arrayCopy(const std::vector<Value>& args)
    {
        if (args.size() != 1) {
            throw errors::RuntimeException("arrayCopy requires 1 argument");
        }

        if (!std::holds_alternative<std::shared_ptr<NativeArray>>(args[0])) {
            throw errors::RuntimeException("arrayCopy requires an array");
        }

        auto array = std::get<std::shared_ptr<NativeArray>>(args[0]);
        return ArrayOperations::copy(array);
    }

    Value ArrayOperationsNative::arrayReverse(const std::vector<Value>& args)
    {
        if (args.size() != 1) {
            throw errors::RuntimeException("arrayReverse requires 1 argument");
        }

        if (!std::holds_alternative<std::shared_ptr<NativeArray>>(args[0])) {
            throw errors::RuntimeException("arrayReverse requires an array");
        }

        auto array = std::get<std::shared_ptr<NativeArray>>(args[0]);
        ArrayOperations::reverse(array);
        return std::monostate{}; // void return
    }

} // namespace runtimeTypes::global
