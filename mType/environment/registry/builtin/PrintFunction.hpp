#pragma once
#include "BuiltinFunction.hpp"
#include "ValuePrinter.hpp"
#include <memory>

namespace environment::registry::builtin
{
    /**
     * @brief Built-in print function
     *
     * Prints all arguments to stdout, separated by spaces, followed by a newline.
     * Handles all Value types including objects with custom toString() methods.
     */
    class PrintFunction : public BuiltinFunction
    {
    private:
        std::unique_ptr<ValuePrinter> printer;

    public:
        explicit PrintFunction(MethodCallHandler methodCallHandler);

        Value execute(const std::vector<Value>& args) override;
        std::string getName() const override;
    };
}
