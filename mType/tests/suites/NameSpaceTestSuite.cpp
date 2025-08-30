#include "NameSpaceTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;
    void NameSpaceTestSuite::setupTests()
    {
        // Basic namespace tests
        addOutputVerificationTest("Basic Namespace Declaration",
                        passPath + "basicNamespace.mt");
        addOutputVerificationTest("Nested Namespaces",
                        passPath + "nestedNamespaces.mt");
        addOutputVerificationTest("Multiple Independent Namespaces",
                        passPath + "multipleNamespaces.mt");
        addOutputVerificationTest("Qualified Function Calls",
                        passPath + "qualifiedFunctionCalls.mt");
        addOutputVerificationTest("Using Directives",
                        passPath + "usingDirectives.mt");
        addOutputVerificationTest("Namespace Variables with Different Types",
                        passPath + "namespaceVariables.mt");
        addOutputVerificationTest("Complex Nested Namespace Structure",
                        passPath + "complexNamespaceStructure.mt");
        addOutputVerificationTest("Namespaces with Different Data Types",
                        passPath + "namespaceWithDifferentTypes.mt");
        addOutputVerificationTest("Empty Namespaces",
                        passPath + "emptyNamespaces.mt");
        addOutputVerificationTest("Deep Namespace Nesting",
                        passPath + "deepNesting.mt");
        addOutputVerificationTest("Namespaces with Control Flow",
                        passPath + "namespaceWithControlFlow.mt");
        addOutputVerificationTest("Namespace Scoping and Variable Shadowing",
                        passPath + "namespaceScoping.mt");
        addOutputVerificationTest("Final Variables in Namespace Declaration",
                        passPath + "namespaceWithFinal.mt");
        addOutputVerificationTest("Using Directives Inside Namespaces",
                        passPath + "usingDirectivesInNamespace.mt");
        
        // Advanced using directive edge case tests
        addOutputVerificationTest("Using Directive Ambiguity Resolution",
                        passPath + "usingDirectiveAmbiguityResolution.mt");
        addOutputVerificationTest("Using Directive Shadowing",
                        passPath + "usingDirectiveShadowing.mt");
        addOutputVerificationTest("Using Directive Nested Scopes",
                        passPath + "usingDirectiveNestedScopes.mt");
        addOutputVerificationTest("Using Directive Order Dependency",
                        passPath + "usingDirectiveOrderDependency.mt");
        addOutputVerificationTest("Using Directive Function Overloading",
                        passPath + "usingDirectiveFunctionOverloading.mt");
        addOutputVerificationTest("Using Directive Scope Isolation",
                        passPath + "usingDirectiveScopeIsolation.mt");
        addOutputVerificationTest("Global Scope Pollution Prevention",
                        passPath + "globalScopePollutionPrevention.mt");

        // Error tests (expected to fail)
        addTestFromFile("Final Variable Modification Error",
                        errorPath + "finalVariableModification.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static Namespace Declaration Error",
                        errorPath + "staticNamespace.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Final Namespace Declaration Error",
                        errorPath + "finalNamespace.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static Using Directive Error",
                        errorPath + "staticUsing.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Final Using Directive Error",
                        errorPath + "finalUsing.mt",
                        TestType::ERROR_EXPECTED);
        
        // Advanced using directive error tests
        addTestFromFile("Using Directive Ambiguous Names Error",
                        errorPath + "usingDirectiveAmbiguousNames.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Using Directive Variable Function Conflict Error",
                        errorPath + "usingDirectiveVariableFunctionConflict.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Using Directive Deep Nesting Conflict Error",
                        errorPath + "usingDirectiveDeepNestingConflict.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Using Directive Same Signature Conflict Error",
                        errorPath + "usingDirectiveSameSignatureConflict.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Using Directive Circular Reference Error",
                        errorPath + "usingDirectiveCircularReference.mt",
                        TestType::ERROR_EXPECTED);
    }
}
