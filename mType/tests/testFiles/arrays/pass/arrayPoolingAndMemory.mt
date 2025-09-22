// Array pooling and memory management tests
function testArrayPoolingAndMemory(): void {
    print("Testing array pooling and memory management");

    // Test 1: Dense vs Sparse storage thresholds
    print("\n=== Test 1: Dense vs Sparse Storage Thresholds ===");

    // Arrays <= 10,000 elements should use dense storage
    print("Creating dense array (100x100 = 10,000 elements)");
    int[][] denseArray = new int[100][100];
    denseArray[50][50] = 999;
    print("Dense array[50][50] = " + denseArray[50][50]);

    // Arrays > 10,000 elements should use sparse storage
    print("Creating sparse array (101x101 = 10,201 elements)");
    int[][] sparseArray = new int[101][101];
    sparseArray[50][50] = 888;
    print("Sparse array[50][50] = " + sparseArray[50][50]);

    // Test 2: Memory reuse patterns
    print("\n=== Test 2: Memory Reuse Patterns ===");

    // Create multiple arrays of same dimensions to test pooling
    print("Creating multiple arrays with same dimensions");
    int[][] array1 = new int[50][50];
    int[][] array2 = new int[50][50];
    int[][] array3 = new int[50][50];

    array1[10][10] = 100;
    array2[20][20] = 200;
    array3[30][30] = 300;

    print("Array1[10][10] = " + array1[10][10]);
    print("Array2[20][20] = " + array2[20][20]);
    print("Array3[30][30] = " + array3[30][30]);

    // Verify arrays are independent
    print("Verifying array independence:");
    print("Array1[20][20] = " + array1[20][20]); // Should be 0
    print("Array2[10][10] = " + array2[10][10]); // Should be 0

    // Test 3: Large array allocation patterns
    print("\n=== Test 3: Large Array Allocation Patterns ===");

    // Test very large sparse arrays
    print("Creating very large sparse array (500x500 = 250,000 elements)");
    int[][] largeArray = new int[500][500];
    largeArray[0][0] = 1;
    largeArray[499][499] = 2;
    print("Large array corners: [0][0] = " + largeArray[0][0] + ", [499][499] = " + largeArray[499][499]);

    // Test 4: Multi-dimensional with varying sizes
    print("\n=== Test 4: Multi-dimensional with Varying Sizes ===");

    print("Testing different dimensional configurations");
    int[][] small2D = new int[10][10];
    int[][] medium2D = new int[20][20];
    int[][][] cube3D = new int[5][5][5];

    small2D[5][5] = 25;
    medium2D[10][10] = 1000;
    cube3D[2][2][2] = 222;

    print("Small 2D[5][5] = " + small2D[5][5]);
    print("Medium 2D[10][10] = " + medium2D[10][10]);
    print("Cube 3D[2][2][2] = " + cube3D[2][2][2]);

    // Test 5: Array cleanup and reallocation
    print("\n=== Test 5: Array Cleanup and Reallocation ===");

    // Create arrays that go out of scope
    for (int i = 0; i < 5; i = i + 1) {
        int[][] tempArray = new int[30][30];
        tempArray[i][i] = i * 10;
        print("Temp array[" + i + "][" + i + "] = " + tempArray[i][i]);
    }

    // Create new arrays after cleanup
    print("Creating new arrays after cleanup");
    int[][] newArray1 = new int[30][30];
    int[][] newArray2 = new int[30][30];
    newArray1[15][15] = 555;
    newArray2[25][25] = 777;
    print("New array1[15][15] = " + newArray1[15][15]);
    print("New array2[25][25] = " + newArray2[25][25]);

    // Test 6: Mixed storage type usage
    print("\n=== Test 6: Mixed Storage Type Usage ===");

    print("Testing mixed dense and sparse arrays");
    int[][] dense1 = new int[50][50];      // Dense storage
    int[][] sparse1 = new int[150][150];   // Sparse storage
    int[][] dense2 = new int[80][80];      // Dense storage
    int[][] sparse2 = new int[200][200];   // Sparse storage

    dense1[25][25] = 2500;
    sparse1[75][75] = 7575;
    dense2[40][40] = 4040;
    sparse2[100][100] = 10000;

    print("Dense1[25][25] = " + dense1[25][25]);
    print("Sparse1[75][75] = " + sparse1[75][75]);
    print("Dense2[40][40] = " + dense2[40][40]);
    print("Sparse2[100][100] = " + sparse2[100][100]);

    // Test 7: Default value handling
    print("\n=== Test 7: Default Value Handling ===");

    print("Testing default value consistency");
    int[][] defaultTest = new int[60][60];
    string[][] stringDefault = new string[40][40];
    bool[][] boolDefault = new bool[30][30];

    print("Int default [10][10] = " + defaultTest[10][10]);
    print("String default [5][5] = '" + stringDefault[5][5] + "'");
    print("Bool default [15][15] = " + boolDefault[15][15]);

    // Verify defaults after assignment
    defaultTest[10][10] = 42;
    stringDefault[5][5] = "test";
    boolDefault[15][15] = true;

    print("After assignment:");
    print("Int [10][10] = " + defaultTest[10][10] + ", [11][11] = " + defaultTest[11][11]);
    print("String [5][5] = '" + stringDefault[5][5] + "', [6][6] = '" + stringDefault[6][6] + "'");
    print("Bool [15][15] = " + boolDefault[15][15] + ", [16][16] = " + boolDefault[16][16]);

    print("\nArray pooling and memory management tests completed");
}

testArrayPoolingAndMemory();