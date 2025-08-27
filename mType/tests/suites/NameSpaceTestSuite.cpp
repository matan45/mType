#include "NameSpaceTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;
    void NameSpaceTestSuite::setupTests()
    {
        // Basic namespace tests
        addTestFromFile("Basic Namespace Declaration",
                        passPath + "basicNamespace.mt");
        addTestFromFile("Nested Namespaces",
                        passPath + "nestedNamespaces.mt");
        addTestFromFile("Multiple Independent Namespaces",
                        passPath + "multipleNamespaces.mt");
        addTestFromFile("Qualified Function Calls",
                        passPath + "qualifiedFunctionCalls.mt");
        addTestFromFile("Using Directives",
                        passPath + "usingDirectives.mt");
        addTestFromFile("Namespace Variables with Different Types",
                        passPath + "namespaceVariables.mt");
        addTestFromFile("Complex Nested Namespace Structure",
                        passPath + "complexNamespaceStructure.mt");
        addTestFromFile("Namespaces with Different Data Types",
                        passPath + "namespaceWithDifferentTypes.mt");
        addTestFromFile("Empty Namespaces",
                        passPath + "emptyNamespaces.mt");
        addTestFromFile("Deep Namespace Nesting",
                        passPath + "deepNesting.mt");
        addTestFromFile("Namespaces with Control Flow",
                        passPath + "namespaceWithControlFlow.mt");
        addTestFromFile("Namespace Scoping and Variable Shadowing",
                        passPath + "namespaceScoping.mt");
        addTestFromFile("Final Variables in Namespace Declaration",
                        passPath + "namespaceWithFinal.mt");
        addTestFromFile("Using Directives Inside Namespaces",
                        passPath + "usingDirectivesInNamespace.mt");
        
        // Advanced using directive edge case tests
        addTestFromFile("Using Directive Ambiguity Resolution",
                        passPath + "usingDirectiveAmbiguityResolution.mt");
        addTestFromFile("Using Directive Shadowing",
                        passPath + "usingDirectiveShadowing.mt");
        addTestFromFile("Using Directive Nested Scopes",
                        passPath + "usingDirectiveNestedScopes.mt");
        addTestFromFile("Using Directive Order Dependency",
                        passPath + "usingDirectiveOrderDependency.mt");
        addTestFromFile("Using Directive Function Overloading",
                        passPath + "usingDirectiveFunctionOverloading.mt");
        addTestFromFile("Using Directive Scope Isolation",
                        passPath + "usingDirectiveScopeIsolation.mt");
        addTestFromFile("Global Scope Pollution Prevention",
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
