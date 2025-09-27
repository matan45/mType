// Array Type Conversion Test
// Tests array handling with new ARRAY ValueType

function testSingleDimensionArrays() :void {
    print("=== Testing Single Dimension Arrays ===");

    // Test basic array creation
    int[] intArray = new int[5];
    intArray[0] = 10;
    intArray[1] = 20;
    intArray[2] = 30;

    print("Int array created with " + intArray.length + " elements");
    print("intArray[0] = " + intArray[0]);
    print("intArray[1] = " + intArray[1]);
    print("intArray[2] = " + intArray[2]);

    // Test string arrays
    string[] stringArray = new string[3];
    stringArray[0] = "First";
    stringArray[1] = "Second";
    stringArray[2] = "Third";

    print("String array:");
    for (int i = 0; i < stringArray.length; i++) {
        print("  [" + i + "] = " + stringArray[i]);
    }

    // Test boolean arrays
    bool[] boolArray = new bool[2];
    boolArray[0] = true;
    boolArray[1] = false;

    print("Bool array:");
    print("  [0] = " + boolArray[0]);
    print("  [1] = " + boolArray[1]);
}

function testMultiDimensionArrays():void {
    print("=== Testing Multi Dimension Arrays ===");

    // Test 2D arrays
    int[][] matrix = new int[3][3];

    // Fill the matrix
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            matrix[i][j] = (i * 3) + j + 1;
        }
    }

    print("2D Matrix (3x3):");
    for (int i = 0; i < 3; i++) {
        string row = "";
        for (int j = 0; j < 3; j++) {
            row = row + matrix[i][j] + " ";
        }
        print("  " + row);
    }
}

function testArrayAssignments():void {
    print("=== Testing Array Assignments ===");

    int[] sourceArray = new int[3];
    sourceArray[0] = 100;
    sourceArray[1] = 200;
    sourceArray[2] = 300;

    // Test array variable assignment
    int[] targetArray = sourceArray;
    print("Array assignment test:");
    print("  Original: [" + sourceArray[0] + ", " + sourceArray[1] + ", " + sourceArray[2] + "]");
    print("  Assigned: [" + targetArray[0] + ", " + targetArray[1] + ", " + targetArray[2] + "]");

    // Modify through one reference
    targetArray[1] = 999;
    print("After modification:");
    print("  Original: [" + sourceArray[0] + ", " + sourceArray[1] + ", " + sourceArray[2] + "]");
    print("  Assigned: [" + targetArray[0] + ", " + targetArray[1] + ", " + targetArray[2] + "]");
}

function testArraysWithObjects():void {
    print("=== Testing Arrays with Objects ===");

    class Person {
        string name;
        int age;

        constructor(string n, int a) {
            this.name = n;
            this.age = a;
        }

        function toString():string {
            return name + "(" + age + ")";
        }
    }

    Person[] people = new Person[3];
    people[0] = new Person("Alice", 25);
    people[1] = new Person("Bob", 30);
    people[2] = new Person("Charlie", 35);

    print("Person array:");
    for (int i = 0; i < people.length; i++) {
        print("  [" + i + "] = " + people[i]);
    }

    // Test null handling in object arrays
    Person[] nullableArray = new Person[2];
    nullableArray[0] = new Person("David", 40);
    nullableArray[1] = null;

    print("Nullable array:");
    for (int i = 0; i < nullableArray.length; i++) {
        if (nullableArray[i] != null) {
            print("  [" + i + "] = " + nullableArray[i]);
        } else {
            print("  [" + i + "] = null");
        }
    }
}

function main():void {
    print("Starting Array Type Tests...");

    testSingleDimensionArrays();
    testMultiDimensionArrays();
    testArrayAssignments();
    testArraysWithObjects();

    print("Array type tests completed!");
}

main();