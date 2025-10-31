#pragma once

#include <cstddef>
#include <cstdint>

namespace debugger
{
    namespace constants
    {
        /// @brief Initial value for reference IDs in variable inspectors
        /// Start at 1000 to avoid confusion with scope handles and other low-numbered IDs
        constexpr int64_t INITIAL_REFERENCE_ID = 1000;

        /// @brief Maximum number of array elements to display in debugger
        /// Limits array expansion to avoid overwhelming the debugger UI
        constexpr size_t MAX_ARRAY_DISPLAY_ELEMENTS = 100;

        /// @brief Maximum reasonable parameter count for validation
        /// Used for defensive programming to detect corrupted call frames
        constexpr size_t MAX_REASONABLE_PARAM_COUNT = 1000;

        /// @brief Maximum reasonable local base value for validation
        /// Used for defensive programming to detect corrupted stack frames
        constexpr size_t MAX_REASONABLE_LOCAL_BASE = 1000000;

        /// @brief Maximum reasonable slot value for validation
        /// Used for defensive programming to detect corrupted variable slots
        constexpr size_t MAX_REASONABLE_SLOT = 10000;

    } // namespace constants
} // namespace debugger
