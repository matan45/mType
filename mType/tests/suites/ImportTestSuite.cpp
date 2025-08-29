#include "ImportTestSuite.hpp"
#include "../testFramework/TestTypeEnum.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void ImportTestSuite::setupTests()
    {
        addTestFromFile("Import System",
                        "tests/testFiles/import/pass/main_script.mt");
        addTestFromFile("Circular Import Detection",
                        "tests/testFiles/import/error/circular_a.mt",
                        TestType::ERROR_EXPECTED);
    }
}
