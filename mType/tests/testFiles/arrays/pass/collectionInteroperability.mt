// Collection interoperability tests
function testCollectionInteroperability(): void {
    print("Testing collection interoperability with arrays");

    // Test 1: Arrays with Set collections
    print("\n=== Test 1: Arrays with Set Collections ===");

    // Create arrays containing different types for sets
    print("Testing arrays with integer sets:");
    int[] intArray = new int[5];
    intArray[0] = 10;
    intArray[1] = 20;
    intArray[2] = 30;
    intArray[3] = 20; // Duplicate for set testing
    intArray[4] = 40;

    print("Integer array contents:");
    for (int i = 0; i < 5; i = i + 1) {
        print("intArray[" + i + "] = " + intArray[i]);
    }

    // Test 2: Arrays with string data for collections
    print("\n=== Test 2: Arrays with String Data ===");

    string[] nameArray = new string[4];
    nameArray[0] = "Alice";
    nameArray[1] = "Bob";
    nameArray[2] = "Charlie";
    nameArray[3] = "Alice"; // Duplicate

    print("String array contents:");
    for (int i = 0; i < 4; i = i + 1) {
        print("nameArray[" + i + "] = " + nameArray[i]);
    }

    // Test 3: Multi-dimensional arrays as collection elements
    print("\n=== Test 3: Multi-dimensional Arrays as Data ===");

    // Array of arrays (matrix-like structure)
    print("Testing matrix-like array structure:");
    int[][] matrix = new int[3][3];

    // Initialize matrix
    for (int i = 0; i < 3; i = i + 1) {
        for (int j = 0; j < 3; j = j + 1) {
            matrix[i][j] = (i + 1) * 10 + (j + 1);
        }
    }

    print("Matrix contents:");
    for (int i = 0; i < 3; i = i + 1) {
        for (int j = 0; j < 3; j = j + 1) {
            print("matrix[" + i + "][" + j + "] = " + matrix[i][j]);
        }
    }

    // Test 4: Arrays with object-like structures
    print("\n=== Test 4: Arrays with Object-like Structures ===");

    // Simulate object arrays using parallel arrays
    print("Simulating person records with parallel arrays:");
    string[] personNames = new string[3];
    int[] personAges = new int[3];
    bool[] personActive = new bool[3];

    // Person 1
    personNames[0] = "John";
    personAges[0] = 25;
    personActive[0] = true;

    // Person 2
    personNames[1] = "Jane";
    personAges[1] = 30;
    personActive[1] = false;

    // Person 3
    personNames[2] = "Bob";
    personAges[2] = 35;
    personActive[2] = true;

    print("Person records:");
    for (int i = 0; i < 3; i = i + 1) {
        print("Person " + i + ": " + personNames[i] + ", age " + personAges[i] + ", active " + personActive[i]);
    }

    // Test 5: Array data aggregation patterns
    print("\n=== Test 5: Array Data Aggregation Patterns ===");

    print("Testing data aggregation from arrays:");
    int[] dataSet = new int[10];

    // Fill with test data
    for (int i = 0; i < 10; i = i + 1) {
        dataSet[i] = (i + 1) * (i + 1); // Square numbers
    }

    // Calculate sum
    int sum = 0;
    for (int i = 0; i < 10; i = i + 1) {
        sum = sum + dataSet[i];
    }

    print("Data set: square numbers 1-10");
    for (int i = 0; i < 10; i = i + 1) {
        print("dataSet[" + i + "] = " + dataSet[i]);
    }
    print("Sum of squares: " + sum);

    // Test 6: Array sorting patterns
    print("\n=== Test 6: Array Sorting Patterns ===");

    print("Testing manual sorting algorithms:");
    int[] unsorted = new int[5];
    unsorted[0] = 64;
    unsorted[1] = 34;
    unsorted[2] = 25;
    unsorted[3] = 12;
    unsorted[4] = 22;

    print("Unsorted array:");
    for (int i = 0; i < 5; i = i + 1) {
        print("unsorted[" + i + "] = " + unsorted[i]);
    }

    // Simple bubble sort
    for (int i = 0; i < 4; i = i + 1) {
        for (int j = 0; j < 4 - i; j = j + 1) {
            if (unsorted[j] > unsorted[j + 1]) {
                // Swap
                int temp = unsorted[j];
                unsorted[j] = unsorted[j + 1];
                unsorted[j + 1] = temp;
            }
        }
    }

    print("Sorted array:");
    for (int i = 0; i < 5; i = i + 1) {
        print("sorted[" + i + "] = " + unsorted[i]);
    }

    // Test 7: Array as lookup table
    print("\n=== Test 7: Array as Lookup Table ===");

    print("Testing array as lookup table:");
    string[] monthNames = new string[12];
    monthNames[0] = "January";
    monthNames[1] = "February";
    monthNames[2] = "March";
    monthNames[3] = "April";
    monthNames[4] = "May";
    monthNames[5] = "June";
    monthNames[6] = "July";
    monthNames[7] = "August";
    monthNames[8] = "September";
    monthNames[9] = "October";
    monthNames[10] = "November";
    monthNames[11] = "December";

    // Lookup examples
    print("Month lookup examples:");
    print("Month 0: " + monthNames[0]);
    print("Month 5: " + monthNames[5]);
    print("Month 11: " + monthNames[11]);

    // Test 8: Sparse array collection patterns
    print("\n=== Test 8: Sparse Array Collection Patterns ===");

    print("Testing sparse array with collection-like usage:");
    int[][] sparseCollection = new int[200][200]; // Sparse storage

    // Simulate sparse key-value pairs
    sparseCollection[10][20] = 1020;    // "key" (10,20) -> value 1020
    sparseCollection[50][75] = 5075;    // "key" (50,75) -> value 5075
    sparseCollection[150][180] = 15180; // "key" (150,180) -> value 15180
    sparseCollection[199][199] = 19999; // "key" (199,199) -> value 19999

    print("Sparse collection entries:");
    print("Entry (10,20): " + sparseCollection[10][20]);
    print("Entry (50,75): " + sparseCollection[50][75]);
    print("Entry (150,180): " + sparseCollection[150][180]);
    print("Entry (199,199): " + sparseCollection[199][199]);

    // Verify empty entries return default
    print("Empty entries (should be 0):");
    print("Entry (0,0): " + sparseCollection[0][0]);
    print("Entry (100,100): " + sparseCollection[100][100]);

    // Test 9: Array iteration patterns
    print("\n=== Test 9: Array Iteration Patterns ===");

    print("Testing various iteration patterns:");
    int[][] iterTest = new int[5][5];

    // Fill with pattern
    for (int i = 0; i < 5; i = i + 1) {
        for (int j = 0; j < 5; j = j + 1) {
            iterTest[i][j] = i * 5 + j;
        }
    }

    // Row-major iteration
    print("Row-major iteration:");
    for (int i = 0; i < 5; i = i + 1) {
        for (int j = 0; j < 5; j = j + 1) {
            print("iterTest[" + i + "][" + j + "] = " + iterTest[i][j]);
        }
    }

    // Test 10: Array bounds and collection safety
    print("\n=== Test 10: Array Bounds and Collection Safety ===");

    print("Testing array bounds in collection contexts:");
    int[] boundTest = new int[3];
    boundTest[0] = 100;
    boundTest[1] = 200;
    boundTest[2] = 300;

    // Safe iteration
    print("Safe iteration:");
    for (int i = 0; i < 3; i = i + 1) {
        print("boundTest[" + i + "] = " + boundTest[i]);
    }

    // Demonstrate collection-like operations
    print("Collection-like operations:");
    int count = 0;
    for (int i = 0; i < 3; i = i + 1) {
        if (boundTest[i] > 150) {
            count = count + 1;
        }
    }
    print("Elements > 150: " + count);

    print("\nCollection interoperability tests completed");
}

testCollectionInteroperability();