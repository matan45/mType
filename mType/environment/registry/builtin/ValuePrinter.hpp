#pragma once
#include <iostream>
#include <optional>
#include <string>
#include <functional>
#include <memory>
#include "../../../value/ValueType.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"

namespace environment::registry::builtin
{
    using namespace value;
    using MethodCallHandler = std::function<Value(std::shared_ptr<runtimeTypes::klass::ObjectInstance>, const std::string&, const std::vector<Value>&)>;

    /**
     * @brief Utility class for printing Value types to output streams
     *
     * Handles the conversion and printing of all Value variant types,
     * including special handling for objects with toString() methods.
     */
    class ValuePrinter
    {
    private:
        MethodCallHandler methodCallHandler;

    public:
        explicit ValuePrinter(MethodCallHandler handler);

        /**
         * @brief Print a value to the given output stream
         * @param value The value to print
         * @param out The output stream (default: std::cout)
         */
        void print(const Value& value, std::ostream& out = std::cout) const;

        /**
         * @brief Get string representation from an object's toString() method
         * @param value The object instance
         * @return Optional string if toString() exists and succeeds, nullopt otherwise
         */
        std::optional<std::string> getObjectStringRepresentation(
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& value) const;

        /**
         * @brief Try to unwrap a primitive wrapper object (String, Int, Bool, Float)
         * @param value The object instance
         * @return Optional Value containing the primitive value if unwrapping succeeds, nullopt otherwise
         */
        std::optional<Value> tryGetPrimitiveWrapperValue(
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& value) const;
    };
}
