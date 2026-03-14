#include "StringInterpolationTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void StringInterpolationTestSuite::setupTests()
    {
        // Pass tests
        addOutputVerificationTest("Basic Interpolation",
                                  passPath + "basicInterpolation.mt");
        addOutputVerificationTest("Expression Interpolation",
                                  passPath + "expressionInterpolation.mt");
        addOutputVerificationTest("Method Call Interpolation",
                                  passPath + "methodCallInterpolation.mt");
        addOutputVerificationTest("Multiple Interpolations",
                                  passPath + "multipleInterpolations.mt");
        addOutputVerificationTest("No Interpolation",
                                  passPath + "noInterpolation.mt");
        addOutputVerificationTest("Type Conversion",
                                  passPath + "typeConversion.mt");
        addOutputVerificationTest("Escaped Braces",
                                  passPath + "escapedBraces.mt");
        addOutputVerificationTest("Empty Segments",
                                  passPath + "emptySegments.mt");

        // Error tests
        addTestFromFile("Empty Interpolation Error",
                        errorPath + "emptyInterpolation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Unterminated Brace Error",
                        errorPath + "unterminatedBrace.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Unterminated String Error",
                        errorPath + "unterminatedString.mt",
                        TestType::ERROR_EXPECTED);
    }
}
