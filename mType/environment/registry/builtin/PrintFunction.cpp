#include "PrintFunction.hpp"
#include "ValuePrinter.hpp"
#include <iostream>

namespace environment::registry::builtin
{
    PrintFunction::PrintFunction(MethodCallHandler methodCallHandler)
        : printer(std::make_unique<ValuePrinter>(methodCallHandler))
    {
    }

    Value PrintFunction::execute(const std::vector<Value>& args)
    {
        for (const auto& arg : args)
        {
            printer->print(arg, std::cout);
        }
        std::cout << std::endl;
        return std::monostate{};
    }

    std::string PrintFunction::getName() const
    {
        return "print";
    }
}
