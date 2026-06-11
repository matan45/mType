#include "NumericsTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void NumericsTestSuite::setupTests()
    {
        // === INT64 BOUNDARIES AND WRAP SEMANTICS ===
        addOutputVerificationTest("Int32 Boundary Literals",
                        passPath + "int32BoundaryLiterals.mt");
        addOutputVerificationTest("Int64 Wrap Add/Sub/Mul",
                        passPath + "int64WrapArithmetic.mt");
        // INT64_MIN % -1 is defined as 0 (Java semantics) via the guarded
        // generic path; the division twin is the MYT-387 skip below.
        addOutputVerificationTest("Int64 Min Mod Minus One",
                        passPath + "int64MinModMinusOne.mt");

        // === MODULO SIGN SEMANTICS ===
        addOutputVerificationTest("Negative Modulo Operands (truncated)",
                        passPath + "negativeModuloOperands.mt");

        // === SHIFTS ===
        addOutputVerificationTest("Shift Boundary Amounts (0 and 63, arithmetic >>)",
                        passPath + "shiftBoundaryAmounts.mt");
        // Hot-loop variant so the JIT pass compiles the shift helpers; the
        // --no-jit pass exercises the interpreter on the same fixture.
        addOutputVerificationTest("Shift Chain Hot Loop",
                        passPath + "shiftChainHotLoop.mt");

        // === BITWISE OPERATORS ===
        addOutputVerificationTest("Bitwise And/Or/Xor Boundary Values",
                        passPath + "bitwiseAndOrXorBoundaries.mt");
        addOutputVerificationTest("Bitwise Not Chain",
                        passPath + "bitwiseNotChain.mt");
        addOutputVerificationTest("Bitwise Result In Condition",
                        passPath + "bitwiseInCondition.mt");

        // === FLOAT SPECIAL VALUES ===
        // NaN/inf are produced via overflow (exponent literals are
        // unsupported); fixtures print boolean discriminators, never raw
        // NaN/inf text, so they are immune to print-formatting changes.
        addOutputVerificationTest("Float NaN Comparison Semantics",
                        passPath + "floatNanComparisons.mt");
        addOutputVerificationTest("Float Overflow Saturates To Inf",
                        passPath + "floatOverflowToInf.mt");

        // === ERROR PATHS ===
        // Runtime arithmetic errors. These pin the executor message text
        // ("Division by zero" etc.) because the MT-E5005 code is shared by
        // all runtime errors and would not discriminate.
        addTestFromFile("Div By Zero (runtime, non-constant)",
                        errorPath + "divByZeroRuntime_error.mt",
                        TestType::ERROR_EXPECTED,
                        "Division by zero");
        addTestFromFile("Modulo By Zero (runtime, non-constant)",
                        errorPath + "moduloByZeroRuntime_error.mt",
                        TestType::ERROR_EXPECTED,
                        "Modulo by zero");
        // BY DESIGN: mType floats do not produce IEEE infinity from division;
        // float x/0.0 is the same runtime error as the int case. (Overflow
        // does saturate to inf — see Float Overflow Saturates To Inf above.)
        addTestFromFile("Float Div By Zero (runtime)",
                        errorPath + "floatDivByZeroRuntime_error.mt",
                        TestType::ERROR_EXPECTED,
                        "Division by zero");
        addTestFromFile("Shift Amount Above 63 Rejected",
                        errorPath + "shiftAmountTooLarge_error.mt",
                        TestType::ERROR_EXPECTED,
                        "Shift amount");
        addTestFromFile("Negative Shift Amount Rejected",
                        errorPath + "shiftAmountNegative_error.mt",
                        TestType::ERROR_EXPECTED,
                        "Shift amount");
        // BY DESIGN pin: VM runtime errors (MT-E5005 family) are a separate
        // channel from script-thrown Exceptions — a script-level try/catch
        // does NOT intercept them. If this test starts failing because the
        // catch fires, that is a deliberate language change, not a regression
        // in the test.
        addTestFromFile("VM Runtime Error Not Catchable By Script",
                        errorPath + "divByZeroNotCatchable_error.mt",
                        TestType::ERROR_EXPECTED,
                        "Division by zero");
        addTestFromFile("Int Literal Beyond Int64 Range Rejected",
                        errorPath + "intLiteralOutOfRange_error.mt",
                        TestType::ERROR_EXPECTED,
                        "out of range");

        // === CANARIES (MYT-385: INT LITERALS TRUNCATED TO 32 BITS) ===
        // ExpressionParser.cpp:863 narrows Token::intValue through
        // static_cast<int>, so any literal above INT32_MAX wraps (INT64_MAX
        // prints as -1). Lexer/Token/IntegerNode/constant pool are all
        // int64-clean; the single cast is the bug. These pin the CORRECT
        // output and stay failing until MYT-385 lands — do not delete or
        // weaken them (memory: feedback_keep_failing_canary_tests).
        addOutputVerificationTest("CANARY Int64 Max Literal",
                        passPath + "int64MaxLiteral.mt");
        addOutputVerificationTest("CANARY Int Literal Above Int32 Max",
                        passPath + "intLiteralAboveInt32Max.mt");

        // === CANARIES (MYT-386: CONSTANT COMPARISON ALWAYS TAKES THEN-BRANCH) ===
        // ConstantFoldingPattern::applyComparison emits PUSH_BOOL whose
        // operand is a constant-pool index, but handlePushBool reads the
        // operand as the inline boolean — a nonzero pool index makes every
        // folded constant comparison read as true, so `if (1 == 2)` executes
        // its body and `while (1 == 2)` loops. Ternary, bool-variable
        // conditions and string comparisons are unaffected. These pin the
        // CORRECT branch selection and stay failing until MYT-386 lands.
        addOutputVerificationTest("CANARY Constant Comparison Branch Selection",
                        passPath + "constantComparisonBranch.mt");
        addOutputVerificationTest("CANARY Dead Branch Constant Condition",
                        passPath + "deadBranchConstantCondition.mt");

        // === INT64 DIVISION EDGE CASES ===
        addOutputVerificationTest("Int64 Min Div Minus One",
                        passPath + "int64MinDivMinusOne.mt");
        addOutputVerificationTest("Int64 Min Div/Mod Minus One Hot",
                        passPath + "int64MinDivModMinusOneHot.mt");
    }
}
