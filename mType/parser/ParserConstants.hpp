#pragma once
#include <cstddef>

namespace parser::constants
{
    /// @brief Maximum number of generic type parameters allowed for classes and interfaces
    /// Prevents excessive template complexity and compilation times
    constexpr size_t MAX_GENERIC_PARAMETERS = 20;
}
