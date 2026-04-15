#pragma once
#include "../environment/Environment.hpp"
#include <memory>
#include <string>

namespace net
{
    // Throws an mType-visible UserException of the appropriate Network*Exception
    // class. `message` may be prefixed with "dns:", "timeout:", or "connection:"
    // to select a subclass; otherwise NetworkException is used.
    [[noreturn]] void throwNetError(std::shared_ptr<environment::Environment> env,
                                    const std::string& message);

    // Same but takes an explicit class name (e.g. "HttpException") and an
    // optional integer field value (e.g. status for HttpException).
    [[noreturn]] void throwTypedNetError(std::shared_ptr<environment::Environment> env,
                                         const std::string& className,
                                         const std::string& message,
                                         int statusCode = 0);
}
