// MYT-126: PrintFunction uses ValuePrinter which is walled off under flag-on.
// Keep the ctor/methods defined so NativeRegistry can still register PrintFunction,
// but under flag-on print() is a no-op stub (benchmarks don't print output).
#include "PrintFunction.hpp"
#include <iostream>
#ifndef MTYPE_TAGGED_VALUE
#include "ValuePrinter.hpp"
#endif

namespace environment::registry::builtin
{
#ifdef MTYPE_TAGGED_VALUE
    PrintFunction::PrintFunction(MethodCallHandler /*methodCallHandler*/)
        : printer(nullptr)
    {
    }

    Value PrintFunction::execute(const std::vector<Value>& /*args*/)
    {
        // stub under flag-on — ValuePrinter walled off
        return std::monostate{};
    }
#else
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
#endif

    std::string PrintFunction::getName() const
    {
        return "print";
    }
}
