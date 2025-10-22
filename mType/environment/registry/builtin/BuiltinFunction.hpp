#pragma once
#include <string>
#include <vector>
#include "../../../value/ValueType.hpp"

namespace environment::registry::builtin
{
    using namespace value;

    /**
     * @brief Interface for all built-in functions
     *
     * This interface defines the contract for built-in functions in the mType language.
     * Each built-in function implements this interface to provide its specific functionality.
     *
     * This follows the Strategy Pattern, allowing easy addition of new built-in functions
     * without modifying the NativeRegistry class.
     */
    class BuiltinFunction
    {
    public:
        virtual ~BuiltinFunction() = default;

        /**
         * @brief Execute the built-in function with the given arguments
         * @param args Vector of argument values
         * @return The result value
         * @throws ArgumentException if arguments are invalid
         * @throws RuntimeException if execution fails
         */
        virtual Value execute(const std::vector<Value>& args) = 0;

        /**
         * @brief Get the name of this built-in function
         * @return The function name as a string
         */
        virtual std::string getName() const = 0;
    };
}
