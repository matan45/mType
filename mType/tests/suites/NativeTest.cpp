#include "NativeTest.hpp"
#include "../../evaluator/Evaluator.hpp"

namespace tests::testSuite
{
    using namespace services;
    using namespace value;

    void NativeTest::setupTests()
    {
        // Initialize the interpreter
        interpreter = std::make_unique<ScriptInterpreter>();
    }

    void NativeTest::runCustomTests()
    {
    }
}
