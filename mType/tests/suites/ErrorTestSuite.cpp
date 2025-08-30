#include "ErrorTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void ErrorTestSuite::setupTests()
    {
        // Basic passing tests for error handling
        addOutputVerificationTest("Valid Error Reporting Code",
                        passPath + "basicErrorReportingValid.mt");
        addOutputVerificationTest("Valid Function Declaration",
                        passPath + "validFunctionDeclaration.mt");
        addOutputVerificationTest("Valid Class Usage",
                        passPath + "validClassUsage.mt");
        addOutputVerificationTest("Valid Type Operations",
                        passPath + "validTypeOperations.mt");
        addOutputVerificationTest("Valid Scope Usage",
                        passPath + "validScopeUsage.mt");

        // Lexer error tests (expected to fail)
        addTestFromFile("Lexer Unterminated String Error",
                        errorPath + "lexerUnteminatedString.mt",
                        TestType::ERROR_EXPECTED);

        // Parser error tests (expected to fail)
        addTestFromFile("Parser Missing Semicolon Error",
                        errorPath + "parserMissingSemicolon.mt",
                        TestType::ERROR_EXPECTED);

        // Syntax error tests (expected to fail)
        addTestFromFile("Syntax Missing Semicolon Error",
                        errorPath + "syntaxMissingSemicolon.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Syntax Unmatched Parentheses Error",
                        errorPath + "syntaxUnmatchedParentheses.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Syntax Invalid Function Name Error",
                        errorPath + "syntaxInvalidFunctionName.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Syntax Missing Return Type Error",
                        errorPath + "syntaxMissingReturnType.mt",
                        TestType::ERROR_EXPECTED);

        // Semantic error tests (expected to fail)
        addTestFromFile("Semantic Undefined Variable Error",
                        errorPath + "semanticUndefinedVariable.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Semantic Variable Redefinition Error",
                        errorPath + "semanticVariableRedefinition.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Semantic Wrong Parameter Count Error",
                        errorPath + "semanticWrongParameterCount.mt",
                        TestType::ERROR_EXPECTED);

        // Runtime error tests (expected to fail)
        addTestFromFile("Runtime Division By Zero Error",
                        errorPath + "runtimeDivisionByZero.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Runtime Null Pointer Dereference Error",
                        errorPath + "runtimeNullPointerDereference.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Missing new Operator For Object Creation",
                        errorPath + "missingNew.mt",
                        TestType::ERROR_EXPECTED);

        // Type error tests (expected to fail)
        addTestFromFile("Type Assignment Mismatch Error",
                        errorPath + "typeAssignmentMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Assign Null To Final",
                        errorPath + "assignNullToFinal.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Invalid Operation Error",
                        errorPath + "typeInvalidOperation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Return Type Mismatch Error",
                        errorPath + "typeReturnTypeMismatch.mt",
                        TestType::ERROR_EXPECTED);

        // Class error tests (expected to fail)
        addTestFromFile("Class Undefined Class Error",
                        errorPath + "classUndefinedClass.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Class Undefined Member Error",
                        errorPath + "classUndefinedMember.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Class Constructor Parameter Mismatch Error",
                        errorPath + "classConstructorParameterMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Class Modifying Final Field Error",
                        errorPath + "classModifyingFinalField.mt",
                        TestType::ERROR_EXPECTED);

        // Scope error tests (expected to fail)
        addTestFromFile("Scope Variable Out Of Scope Error",
                        errorPath + "scopeVariableOutOfScope.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Scope Function Parameter Shadowing Error",
                        errorPath + "scopeFunctionParameterShadowing.mt",
                        TestType::ERROR_EXPECTED);

        // Import error tests (expected to fail)
        addTestFromFile("Import Non Existent File Error",
                        errorPath + "importNonExistentFile.mt",
                        TestType::ERROR_EXPECTED);

        // Enhanced exception location tests (expected to fail with location info)
        addTestFromFile("Enhanced Division By Zero Location Error",
                        errorPath + "enhancedDivisionByZeroLocation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Enhanced Type Mismatch Location Error",
                        errorPath + "enhancedTypeMismatchLocation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Enhanced Undefined Function Location Error",
                        errorPath + "enhancedUndefinedFunctionLocation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Enhanced Static Method Error Location",
                        errorPath + "enhancedStaticMethodErrorLocation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Enhanced Argument Mismatch Location Error",
                        errorPath + "enhancedArgumentMismatchLocation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Enhanced Final Variable Modification Location Error",
                        errorPath + "enhancedFinalVariableModificationLocation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Enhanced Null Pointer Location Error",
                        errorPath + "enhancedNullPointerLocation.mt",
                        TestType::ERROR_EXPECTED);
    }
}
