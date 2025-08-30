#pragma once

#include "../../services/ScriptInterpreter.hpp"

namespace tests::testSuite
{
    class NativeTest 
    {
    private:
        std::unique_ptr<services::ScriptInterpreter> interpreter;
        
    public:
        explicit NativeTest() {}
        void setupTests();
        void runCustomTests();
    };
}