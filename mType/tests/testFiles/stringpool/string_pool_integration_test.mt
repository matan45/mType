// StringPool Integration Test
// Tests string pool integration with all mType language features

string errorMessage = "test_error_message";
        string errorType = "TestException";

    function testNamespaceStringPooling(): void {
        print("=== Namespace String Pooling Test ===");

        string namespaceName = "StringPoolTests";
        string functionName = "testNamespaceStringPooling";

        print("Testing namespace and function names:");
        print("Namespace: " + namespaceName);
        print("Function: " + functionName);

        // Test qualified names
        string qualifiedName = namespaceName + "." + functionName;
        print("Qualified name: " + qualifiedName);

        print("Namespace string pooling test completed");
    }

    class StringPoolTestClass {
        string className;
        string memberField;
        static string staticField = "shared_static_field";

        function init(string name): void {
            this.className = name;
            this.memberField = "member_" + name;
        }

        function testMethodStrings(): void {
            string methodName = "testMethodStrings";
            string localVar = "local_variable";

            print("Method string testing:");
            print("Class: " + this.className);
            print("Member: " + this.memberField);
            print("Method: " + methodName);
            print("Local: " + localVar);

            return methodName + " completed";
        }

        static function testStaticMethodStrings(): void {
            string staticMethodName = "testStaticMethodStrings";
            string staticLocal = "static_local_var";

            print("Static method string testing:");
            print("Method: " + staticMethodName);
            print("Local: " + staticLocal);

            return staticMethodName + " completed";
        }

        function testStringReturnTypes(): void {
            return "string_return_value";
        }
    }

    function testClassStringPooling(): void {
        print("=== Class String Pooling Test ===");

        StringPoolTestClass obj1 = new StringPoolTestClass();
        obj1.init("TestClass1");

        StringPoolTestClass obj2 = new StringPoolTestClass();
        obj2.init("TestClass1");  // Same class name, should be pooled

        string result1 = obj1.testMethodStrings();
        string result2 = obj2.testMethodStrings();

        print("Object method results:");
        print("Result 1: " + result1);
        print("Result 2: " + result2);

        string staticResult = StringPoolTestClass::testStaticMethodStrings();
        print("Static method result: " + staticResult);

        // Test return values
        string returnVal1 = obj1.testStringReturnTypes();
        string returnVal2 = obj2.testStringReturnTypes();

        print("Return values:");
        print("Return 1: " + returnVal1);
        print("Return 2: " + returnVal2);
        print("Returns match: " + (returnVal1 == returnVal2));

        print("Class string pooling test completed");
    }

    function testControlFlowStringPooling(): void {
        print("=== Control Flow String Pooling Test ===");

        string[] conditions = ["condition1", "condition2", "condition3"];
        string[] results = new string[3];

        for (int i = 0; i < 3; i = i + 1) {
            string currentCondition = conditions[i];

            if (currentCondition == "condition1") {
                results[i] = "result_for_condition1";
            } else if (currentCondition == "condition2") {
                results[i] = "result_for_condition2";
            } else {
                results[i] = "default_result";
            }

            print("Processing: " + currentCondition + " -> " + results[i]);
        }

        // Test switch-like behavior with strings
        string testValue = "test_case";
        string switchResult;

        if (testValue == "test_case") {
            switchResult = "matched_test_case";
        } else if (testValue == "other_case") {
            switchResult = "matched_other_case";
        } else {
            switchResult = "no_match";
        }

        print("Switch-like result: " + switchResult);

        // Test ternary operations with strings
        string ternaryResult = (testValue == "test_case") ? "ternary_true" : "ternary_false";
        print("Ternary result: " + ternaryResult);

        print(" Control flow string pooling test completed");
    }

    function testArrayStringPooling(): void {
        print("=== Array String Pooling Test ===");

        // Test arrays of strings
        string[] stringArray = ["array_element_1", "array_element_2", "array_element_3"];
        string[] duplicateArray = ["array_element_1", "array_element_2", "array_element_3"];

        print("String arrays created:");
        for (int i = 0; i < 3; i = i + 1) {
            print("Index " + i + ": " + stringArray[i]);
            if (stringArray[i] == duplicateArray[i]) {
                print("   Array element " + i + " matches duplicate");
            }
        }

        // Test multi-dimensional arrays with strings
        string[][] matrix = new string[2][2];
        matrix[0][0] = "matrix_00";
        matrix[0][1] = "matrix_01";
        matrix[1][0] = "matrix_10";
        matrix[1][1] = "matrix_11";

        print("String matrix:");
        for (int i = 0; i < 2; i = i + 1) {
            for (int j = 0; j < 2; j = j + 1) {
                print("  [" + i + "][" + j + "]: " + matrix[i][j]);
            }
        }

        print(" Array string pooling test completed");
    }

    function testImportStringPooling(): void {
        print("=== Import/Using String Pooling Test ===");

        // Simulate import-like behavior
        string moduleName = "StringUtilities";
        string importedFunction = "formatString";
        string qualifiedCall = moduleName + "." + importedFunction;

        print("Import simulation:");
        print("Module: " + moduleName);
        print("Function: " + importedFunction);
        print("Qualified call: " + qualifiedCall);

        // Test using directive simulation
        string usingDirective = "using " + moduleName;
        print("Using directive: " + usingDirective);

        print(" Import string pooling test completed");
    }

function throwTestException() :bool {
            // Simulate throwing an exception with string message
            print("Throwing: " + errorType + " with message: " + errorMessage);
            return false; // Simulate exception condition
        }

    function testExceptionStringPooling(): void {
        print("=== Exception String Pooling Test ===");

        

        // Simulate exception handling
        bool exceptionThrown = false;

        

        if (!throwTestException()) {
            exceptionThrown = true;
            string handledMessage = "Handled " + errorType + ": " + errorMessage;
            print("Exception handled: " + handledMessage);
        }

        if (exceptionThrown) {
            print(" Exception string handling completed");
        }

        print(" Exception string pooling test completed");
    }


function testStringPoolIntegration(): void {
    print("=== StringPool Integration Test Suite ===");

    testNamespaceStringPooling();
    testClassStringPooling();
    testControlFlowStringPooling();
    testArrayStringPooling();
    testImportStringPooling();
    testExceptionStringPooling();

    print(" All integration tests completed");
}

// Test generic-like behavior with strings
    function createTypedContainer(string typeName): string {
        string containerType = "Container<" + typeName + ">";
        print("Created container: " + containerType);
        return containerType;
    }

function testStringPoolWithGenerics(): void {
    print("=== String Pool with Generics Test ===");

    

    string intContainer = createTypedContainer("int");
    string stringContainer = createTypedContainer("string");
    string objectContainer = createTypedContainer("Object");

    print("Generic containers:");
    print("Int: " + intContainer);
    print("String: " + stringContainer);
    print("Object: " + objectContainer);

    // Test repeated generic instantiations
    string intContainer2 = createTypedContainer("int");
    if (intContainer == intContainer2) {
        print(" Generic type strings match correctly");
    }

    print(" String pool with generics test completed");
}

function testStringPoolWithCollections(): void {
    print("=== String Pool with Collections Test ===");

    // Test collection types with strings (if supported)
    string[] collectionTypes = ["List", "Set", "Map", "Queue", "Stack"];
    string[] elementTypes = ["string", "int", "Object"];

    for (int i = 0; i < 5; i = i + 1) {
        for (int j = 0; j < 3; j = j + 1) {
            string collectionDecl = collectionTypes[i] + "<" + elementTypes[j] + ">";
            print("Collection type: " + collectionDecl);
        }
    }

    print(" String pool with collections test completed");
}

// Entry point
testStringPoolIntegration();
testStringPoolWithGenerics();
testStringPoolWithCollections();

print("=== String Pool Integration Test Suite Completed ===");