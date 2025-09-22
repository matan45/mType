// Performance characteristics tests for large arrays
function testPerformanceCharacteristics(): void {
    print("Testing performance characteristics of large arrays");

    // Test 1: Dense vs Sparse performance patterns
    print("\n=== Test 1: Dense vs Sparse Performance Patterns ===");

    print("Dense array performance (100x100):");
    int[][] densePerf = new int[100][100];

    // Sequential access pattern (cache-friendly for dense)
    for (int i = 0; i < 100; i = i + 1) {
        for (int j = 0; j < 100; j = j + 1) {
            densePerf[i][j] = i * 100 + j;
        }
    }
    print("Dense sequential write completed");

    // Verify a few values
    print("Dense[0][0] = " + densePerf[0][0]);
    print("Dense[50][50] = " + densePerf[50][50]);
    print("Dense[99][99] = " + densePerf[99][99]);

    print("Sparse array performance (150x150):");
    int[][] sparsePerf = new int[150][150];

    // Scattered access pattern (efficient for sparse)
    sparsePerf[10][20] = 1020;
    sparsePerf[50][75] = 5075;
    sparsePerf[100][120] = 100120;
    sparsePerf[149][149] = 149149;
    print("Sparse scattered write completed");

    print("Sparse[10][20] = " + sparsePerf[10][20]);
    print("Sparse[50][75] = " + sparsePerf[50][75]);
    print("Sparse[100][120] = " + sparsePerf[100][120]);
    print("Sparse[149][149] = " + sparsePerf[149][149]);

    // Test 2: Large array initialization patterns
    print("\n=== Test 2: Large Array Initialization Patterns ===");

    print("Testing various large array sizes:");

    // Boundary threshold testing
    print("Creating 99x99 array (9,801 elements - should be dense)");
    int[][] boundary1 = new int[99][99];
    boundary1[98][98] = 9801;
    print("Boundary1[98][98] = " + boundary1[98][98]);

    print("Creating 100x100 array (10,000 elements - should be dense)");
    int[][] boundary2 = new int[100][100];
    boundary2[99][99] = 10000;
    print("Boundary2[99][99] = " + boundary2[99][99]);

    print("Creating 101x101 array (10,201 elements - should be sparse)");
    int[][] boundary3 = new int[101][101];
    boundary3[100][100] = 10201;
    print("Boundary3[100][100] = " + boundary3[100][100]);

    print("Creating 200x200 array (40,000 elements - should be sparse)");
    int[][] large1 = new int[200][200];
    large1[199][199] = 40000;
    print("Large1[199][199] = " + large1[199][199]);

    // Test 3: Access pattern performance
    print("\n=== Test 3: Access Pattern Performance ===");

    print("Testing different access patterns:");
    int[][] accessTest = new int[120][120];

    // Diagonal access pattern
    print("Diagonal access pattern:");
    for (int i = 0; i < 120; i = i + 10) {
        accessTest[i][i] = i * i;
    }
    for (int i = 0; i < 120; i = i + 10) {
        print("Diagonal[" + i + "][" + i + "] = " + accessTest[i][i]);
    }

    // Random sparse access pattern
    print("Random sparse access pattern:");
    accessTest[5][95] = 595;
    accessTest[25][115] = 25115;
    accessTest[45][35] = 4535;
    accessTest[85][15] = 8515;
    accessTest[115][75] = 11575;

    print("Random[5][95] = " + accessTest[5][95]);
    print("Random[25][115] = " + accessTest[25][115]);
    print("Random[45][35] = " + accessTest[45][35]);
    print("Random[85][15] = " + accessTest[85][15]);
    print("Random[115][75] = " + accessTest[115][75]);

    // Test 4: Memory usage patterns
    print("\n=== Test 4: Memory Usage Patterns ===");

    print("Testing memory usage with sparse patterns:");
    int[][] memTest = new int[300][300]; // 90,000 elements - sparse

    // Test very sparse usage (only a few elements)
    memTest[0][0] = 1;
    memTest[150][150] = 2;
    memTest[299][299] = 3;
    print("Sparse usage: only 3 out of 90,000 elements set");
    print("Corner values: " + memTest[0][0] + ", " + memTest[150][150] + ", " + memTest[299][299]);

    // Test medium density usage
    print("Medium density test:");
    for (int i = 0; i < 300; i = i + 30) {
        for (int j = 0; j < 300; j = j + 30) {
            memTest[i][j] = i + j;
        }
    }
    print("Set 100 out of 90,000 elements (0.11% density)");
    print("Sample values: " + memTest[60][60] + ", " + memTest[120][120] + ", " + memTest[240][240]);

    // Test 5: Multi-dimensional scaling
    print("\n=== Test 5: Multi-dimensional Scaling ===");

    print("Testing 2D and 3D array configurations:");
    int[][] square2D = new int[30][30]; // 900 elements
    int[][] rect2D = new int[20][50]; // 1,000 elements
    int[][][] cube3D = new int[10][10][10]; // 1,000 elements

    square2D[15][15] = 151515;
    rect2D[10][25] = 1025;
    cube3D[5][5][5] = 555;

    print("Square 2D center: " + square2D[15][15]);
    print("Rect 2D center: " + rect2D[10][25]);
    print("Cube 3D center: " + cube3D[5][5][5]);

    // Test 6: Performance under load
    print("\n=== Test 6: Performance Under Load ===");

    print("Testing multiple large arrays simultaneously:");
    int[][] load1 = new int[180][180]; // 32,400 elements
    int[][] load2 = new int[190][190]; // 36,100 elements
    int[][] load3 = new int[200][200]; // 40,000 elements

    // Set values in all arrays
    load1[90][90] = 90;
    load2[95][95] = 95;
    load3[100][100] = 100;

    // Interleaved access
    for (int i = 0; i < 10; i = i + 1) {
        load1[i][i] = i;
        load2[i + 10][i + 10] = i + 10;
        load3[i + 20][i + 20] = i + 20;
    }

    print("Load test results:");
    print("Load1[90][90] = " + load1[90][90] + ", Load1[5][5] = " + load1[5][5]);
    print("Load2[95][95] = " + load2[95][95] + ", Load2[15][15] = " + load2[15][15]);
    print("Load3[100][100] = " + load3[100][100] + ", Load3[25][25] = " + load3[25][25]);

    // Test 7: Threshold boundary behavior
    print("\n=== Test 7: Threshold Boundary Behavior ===");

    print("Testing arrays around sparse threshold:");

    // Just under threshold
    int[][] justUnder = new int[99][101]; // 9,999 elements
    justUnder[50][50] = 9999;
    print("Just under (99x101=9,999): " + justUnder[50][50]);

    // Just over threshold
    int[][] justOver = new int[100][101]; // 10,100 elements
    justOver[50][50] = 10100;
    print("Just over (100x101=10,100): " + justOver[50][50]);

    // Exact threshold
    int[][] exactThreshold = new int[100][100]; // 10,000 elements
    exactThreshold[50][50] = 10000;
    print("Exact threshold (100x100=10,000): " + exactThreshold[50][50]);

    print("\nPerformance characteristics tests completed");
}

testPerformanceCharacteristics();