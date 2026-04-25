#include "NativeContext.hpp"
#include "Environment.hpp"
#include "../errors/RuntimeException.hpp"

namespace environment {
    void NativeContext::throwException(const std::string& type, const std::string& message) {
        // In a real implementation, this would likely look up the mType exception class
        // and throw a C++ exception that the VM can catch.
        throw errors::RuntimeException(type + ": " + message);
    }
}
