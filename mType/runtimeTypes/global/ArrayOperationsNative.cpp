#include "ArrayOperationsNative.hpp"
#include "../../value/operations/ArrayOperations.hpp"
#include "../../value/NativeArray.hpp"
#include "../../value/ValueShim.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../environment/registry/NativeBinder.hpp"

namespace runtimeTypes::global
{
    using namespace value;
    using namespace value::operations;
    using namespace environment::registry;

    void ArrayOperationsNative::registerAll(std::shared_ptr<environment::Environment> env)
    {
        auto nativeRegistry = env->getNativeRegistry();

        // Arithmetic operations
        nativeRegistry->registerNativeFunction("arrayAdd", NativeBinder::bind(ArrayOperations::add));
        nativeRegistry->registerNativeFunction("arrayAddScalar", NativeBinder::bind(ArrayOperations::addScalar));
        nativeRegistry->registerNativeFunction("arraySubtract", NativeBinder::bind(ArrayOperations::subtract));
        nativeRegistry->registerNativeFunction("arrayMultiply", NativeBinder::bind(ArrayOperations::multiply));
        nativeRegistry->registerNativeFunction("arrayMultiplyScalar", NativeBinder::bind(ArrayOperations::multiplyScalar));

        // Reduction operations
        nativeRegistry->registerNativeFunction("arraySum", NativeBinder::bind(ArrayOperations::sum));
        nativeRegistry->registerNativeFunction("arrayMin", NativeBinder::bind(ArrayOperations::min));
        nativeRegistry->registerNativeFunction("arrayMax", NativeBinder::bind(ArrayOperations::max));
        nativeRegistry->registerNativeFunction("arrayAverage", NativeBinder::bind(ArrayOperations::average));

        // Utility operations
        nativeRegistry->registerNativeFunction("arrayFill", NativeBinder::bind(ArrayOperations::fill));
        nativeRegistry->registerNativeFunction("arrayCopy", NativeBinder::bind(ArrayOperations::copy));
        nativeRegistry->registerNativeFunction("arrayReverse", NativeBinder::bind(ArrayOperations::reverse));
    }
}
