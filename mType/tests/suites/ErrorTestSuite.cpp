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
        addOutputVerificationTest("Valid Inner Scope Blocks",
                        passPath + "validInnerScope.mt");

        // Lexer error tests (expected to fail) - FIXED: No longer causes infinite loop
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
        addTestFromFile("Nested Function Declaration Error",
                        errorPath + "nestedFunctionDeclaration.mt",
                        TestType::ERROR_EXPECTED);

        // Semantic error tests (expected to fail)
        addTestFromFile("Semantic Undefined Variable Error",
                        errorPath + "semanticUndefinedVariable.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Semantic Variable Redefinition Error",
                        errorPath + "semanticVariableRedefinition.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Undefined Variable Reassignment Error",
                        errorPath + "undefinedVariableReassignment.mt",
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

        // Static method validation error tests (expected to fail)
        addTestFromFile("Static Method Access This Error",
                        errorPath + "staticMethodAccessThis.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static Method Access Instance Field Error",
                        errorPath + "staticMethodAccessInstanceField.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static Method Access Instance Member Error",
                        errorPath + "staticMethodAccessInstanceMember.mt",
                        TestType::ERROR_EXPECTED);

        // === IDENTIFIER VALIDATION TESTS ===
        // These tests verify that identifiers with special characters are rejected

        addTestFromFile("Invalid Variable Name with Special Character Error",
                        errorPath + "invalidVariableNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Class Name with Special Character Error",
                        errorPath + "invalidClassNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Interface Name with Special Character Error",
                        errorPath + "invalidInterfaceNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Function Name with Special Character Error",
                        errorPath + "invalidFunctionNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Field Name with Special Character Error",
                        errorPath + "invalidFieldNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Parameter Name with Special Character Error",
                        errorPath + "invalidParameterNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Method Name with Special Character Error",
                        errorPath + "invalidMethodNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Static Variable Name with Special Character Error",
                        errorPath + "invalidStaticVarNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Import Symbol Name with Special Character Error",
                        errorPath + "invalidImportSymbolNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);

        // === DUPLICATE TYPE DECLARATION TESTS ===
        // These tests verify that duplicate class/interface names are rejected
        addTestFromFile("Duplicate Class Name Error",
                        errorPath + "duplicateClassName.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Duplicate Interface Name Error",
                        errorPath + "duplicateInterfaceName.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Duplicate Class and Interface Name Error",
                        errorPath + "duplicateClassInterfaceName.mt",
                        TestType::ERROR_EXPECTED);

        // === DUPLICATE FUNCTION DECLARATION TESTS ===
        // These tests verify that duplicate global function names are rejected
        addTestFromFile("Duplicate Function Name Error",
                        errorPath + "duplicateFunctionName.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Duplicate Native Function Name Error",
                        errorPath + "duplicateNativeFunctionName.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Duplicate Function and Native Function Name Error",
                        errorPath + "duplicateFunctionNativeName.mt",
                        TestType::ERROR_EXPECTED);

        // === DUPLICATE METHOD DECLARATION TESTS ===
        // These tests verify that duplicate method names within a class are rejected
        addTestFromFile("Duplicate Instance Method Name Error",
                        errorPath + "duplicateInstanceMethodName.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Duplicate Static Method Name Error",
                        errorPath + "duplicateStaticMethodName.mt",
                        TestType::ERROR_EXPECTED);

        // Test that static and instance methods with same name are allowed (different namespaces)
        addOutputVerificationTest("Allow Static And Instance Method Same Name",
                        passPath + "allowStaticAndInstanceSameName.mt");
    }
}
