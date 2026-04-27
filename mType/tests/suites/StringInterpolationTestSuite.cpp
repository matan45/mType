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
        addOutputVerificationTest("Array Interpolation",
                                  passPath + "arrayInterpolation.mt");
        addOutputVerificationTest("Object Interpolation",
                                  passPath + "objectInterpolation.mt");
        addOutputVerificationTest("Whitespace Interpolation",
                                  passPath + "whitespaceInterpolation.mt");
        addOutputVerificationTest("Inner String Literal",
                                  passPath + "innerStringLiteral.mt");
        addOutputVerificationTest("STRING_BUILD Optimization",
                                  passPath + "stringBuildOptimization.mt");
        addOutputVerificationTest("Multi-Dimensional Array Interpolation",
                                  passPath + "multiDimArrayInterpolation.mt");

        // === CROSS-FEATURE EDGE CASES ===
        addOutputVerificationTest("Cast Inside Interpolation",
                                  passPath + "castInsideInterpolation.mt");

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
