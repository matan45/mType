// Edge cases in multi-dimensional indexing tests
function testMultiDimensionalEdgeCases(): void {
    print("Testing edge cases in multi-dimensional indexing");

    // Test 1: Boundary index access
    print("\n=== Test 1: Boundary Index Access ===");

    print("Testing boundary indices on various array sizes:");
    int[][] small = new int[2][2];
    int[][] medium = new int[10][10];
    int[][] large = new int[150][150]; // Sparse array

    // Test corner access (0,0)
    small[0][0] = 11;
    medium[0][0] = 101;
    large[0][0] = 1001;

    // Test maximum index access
    small[1][1] = 22;
    medium[9][9] = 999;
    large[149][149] = 149149;

    print("Corner (0,0) values:");
    print("Small[0][0] = " + small[0][0]);
    print("Medium[0][0] = " + medium[0][0]);
    print("Large[0][0] = " + large[0][0]);

    print("Maximum index values:");
    print("Small[1][1] = " + small[1][1]);
    print("Medium[9][9] = " + medium[9][9]);
    print("Large[149][149] = " + large[149][149]);

    // Test 2: Single element arrays
    print("\n=== Test 2: Single Element Arrays ===");

    print("Testing single element arrays:");
    int[][] single2D = new int[1][1];
    int[][] tiny2D = new int[2][1];
    int[][] micro2D = new int[1][2];

    single2D[0][0] = 100;
    tiny2D[1][0] = 1000;
    micro2D[0][1] = 10000;

    print("Single 2D[0][0] = " + single2D[0][0]);
    print("Tiny 2D[1][0] = " + tiny2D[1][0]);
    print("Micro 2D[0][1] = " + micro2D[0][1]);

    // Test 3: Asymmetric arrays
    print("\n=== Test 3: Asymmetric Arrays ===");

    print("Testing asymmetric multi-dimensional arrays:");
    int[][] rect1 = new int[3][7];    // 3x7 rectangle
    int[][] rect2 = new int[8][2];    // 8x2 rectangle
    int[][][] asym3D = new int[5][3][4]; // 5x3x4 box

    // Fill asymmetric patterns
    rect1[0][6] = 6;    // First row, last column
    rect1[2][0] = 20;   // Last row, first column
    rect1[1][3] = 13;   // Middle

    rect2[7][1] = 71;   // Last row, last column
    rect2[0][0] = 0;    // First corner
    rect2[4][1] = 41;   // Middle

    asym3D[4][2][3] = 423; // Corner
    asym3D[0][0][0] = 0;   // Origin
    asym3D[2][1][2] = 212; // Center-ish

    print("Asymmetric array access:");
    print("Rect1[0][6] = " + rect1[0][6] + ", Rect1[2][0] = " + rect1[2][0] + ", Rect1[1][3] = " + rect1[1][3]);
    print("Rect2[7][1] = " + rect2[7][1] + ", Rect2[0][0] = " + rect2[0][0] + ", Rect2[4][1] = " + rect2[4][1]);
    print("Asym3D[4][2][3] = " + asym3D[4][2][3] + ", Asym3D[0][0][0] = " + asym3D[0][0][0] + ", Asym3D[2][1][2] = " + asym3D[2][1][2]);

    // Test 4: 3D arrays
    print("\n=== Test 4: 3D Arrays ===");

    print("Testing 3D arrays:");
    int[][][] cube3D = new int[3][3][3]; // 3D cube

    // Test various access patterns
    cube3D[0][0][0] = 0;     // Origin
    cube3D[2][2][2] = 222;   // Far corner
    cube3D[1][0][2] = 102;   // Mixed indices
    cube3D[0][2][1] = 21;    // Another pattern

    print("3D array access:");
    print("Cube3D[0][0][0] = " + cube3D[0][0][0]);
    print("Cube3D[2][2][2] = " + cube3D[2][2][2]);
    print("Cube3D[1][0][2] = " + cube3D[1][0][2]);
    print("Cube3D[0][2][1] = " + cube3D[0][2][1]);

    // Test 5: Sparse vs Dense threshold edge cases
    print("\n=== Test 5: Sparse vs Dense Threshold Edge Cases ===");

    print("Testing around sparse/dense thresholds:");

    // Exactly at threshold
    int[][] exactThreshold = new int[100][100]; // Exactly 10,000 elements
    exactThreshold[50][50] = 5000;
    exactThreshold[99][99] = 9999;
    print("Exact threshold (100x100): [50][50] = " + exactThreshold[50][50] + ", [99][99] = " + exactThreshold[99][99]);

    // Just over threshold
    int[][] justOver = new int[100][101]; // 10,100 elements
    justOver[0][100] = 100;
    justOver[99][100] = 99100;
    print("Just over threshold (100x101): [0][100] = " + justOver[0][100] + ", [99][100] = " + justOver[99][100]);

    // Way over threshold
    int[][] wayOver = new int[316][317]; // ~100,000 elements
    wayOver[0][0] = 1;
    wayOver[315][316] = 315316;
    wayOver[158][158] = 158158; // Center
    print("Way over threshold (316x317): [0][0] = " + wayOver[0][0] + ", [315][316] = " + wayOver[315][316] + ", [158][158] = " + wayOver[158][158]);

    // Test 6: Mixed access patterns on large arrays
    print("\n=== Test 6: Mixed Access Patterns on Large Arrays ===");

    print("Testing mixed access patterns:");
    int[][] mixedAccess = new int[200][200]; // Sparse array

    // Sequential pattern
    for (int i = 0; i < 5; i = i + 1) {
        mixedAccess[i][i] = i * 10;
    }

    // Random sparse pattern
    mixedAccess[50][150] = 50150;
    mixedAccess[175][25] = 17525;
    mixedAccess[199][0] = 19900;
    mixedAccess[0][199] = 199;

    // Corner pattern
    mixedAccess[0][0] = 1;
    mixedAccess[0][199] = 2;
    mixedAccess[199][0] = 3;
    mixedAccess[199][199] = 4;

    print("Sequential diagonal:");
    for (int i = 0; i < 5; i = i + 1) {
        print("mixedAccess[" + i + "][" + i + "] = " + mixedAccess[i][i]);
    }

    print("Random sparse positions:");
    print("mixedAccess[50][150] = " + mixedAccess[50][150]);
    print("mixedAccess[175][25] = " + mixedAccess[175][25]);
    print("mixedAccess[199][0] = " + mixedAccess[199][0]);
    print("mixedAccess[0][199] = " + mixedAccess[0][199]);

    print("Corner pattern:");
    print("Corners: " + mixedAccess[0][0] + ", " + mixedAccess[0][199] + ", " + mixedAccess[199][0] + ", " + mixedAccess[199][199]);

    // Test 7: Nested loop edge cases
    print("\n=== Test 7: Nested Loop Edge Cases ===");

    print("Testing nested loops with edge indices:");
    int[][] nestedTest = new int[10][10];

    // Fill with nested loop, testing boundaries
    for (int i = 0; i < 10; i = i + 1) {
        for (int j = 0; j < 10; j = j + 1) {
            if (i == 0 || i == 9 || j == 0 || j == 9) {
                nestedTest[i][j] = 999; // Border elements
            } else {
                nestedTest[i][j] = i * 10 + j; // Interior elements
            }
        }
    }

    print("Border and interior pattern:");
    print("Corners: " + nestedTest[0][0] + ", " + nestedTest[0][9] + ", " + nestedTest[9][0] + ", " + nestedTest[9][9]);
    print("Borders: " + nestedTest[0][5] + ", " + nestedTest[5][0] + ", " + nestedTest[9][5] + ", " + nestedTest[5][9]);
    print("Interior: " + nestedTest[1][1] + ", " + nestedTest[5][5] + ", " + nestedTest[8][8]);

    // Test 8: Index calculation edge cases
    print("\n=== Test 8: Index Calculation Edge Cases ===");

    print("Testing complex index calculations:");
    int[][] calcTest = new int[20][30]; // 20 rows, 30 columns

    // Test calculated indices
    int row = 5;
    int col = 10;
    calcTest[row][col] = row * 100 + col;

    int maxRow = 19;
    int maxCol = 29;
    calcTest[maxRow][maxCol] = maxRow * 100 + maxCol;

    // Test arithmetic index access
    calcTest[2 * 3][4 * 5] = 2345;
    calcTest[15 + 2][25 + 3] = 1528;

    print("Calculated index access:");
    print("calcTest[5][10] = " + calcTest[5][10]);
    print("calcTest[19][29] = " + calcTest[19][29]);
    print("calcTest[6][20] = " + calcTest[6][20]); // 2*3, 4*5
    print("calcTest[17][28] = " + calcTest[17][28]); // 15+2, 25+3

    // Test 9: Default value consistency in edge cases
    print("\n=== Test 9: Default Value Consistency ===");

    print("Testing default values in edge cases:");
    int[][] defaultTest = new int[100][100];
    string[][] stringDefTest = new string[50][50];
    bool[][] boolDefTest = new bool[75][75];

    // Set one value and check neighbors
    defaultTest[50][50] = 42;
    stringDefTest[25][25] = "set";
    boolDefTest[37][37] = true;

    print("Default value verification:");
    print("Int: set[50][50] = " + defaultTest[50][50] + ", neighbor[49][49] = " + defaultTest[49][49]);
    print("String: set[25][25] = '" + stringDefTest[25][25] + "', neighbor[24][24] = '" + stringDefTest[24][24] + "'");
    print("Bool: set[37][37] = " + boolDefTest[37][37] + ", neighbor[36][36] = " + boolDefTest[36][36]);

    // Test 10: Complex chained access
    print("\n=== Test 10: Complex Chained Access ===");

    print("Testing complex chained index access:");
    int[][][] complex3D = new int[15][20][25];

    // Set values using complex patterns
    complex3D[0][0][0] = 1;          // Origin
    complex3D[14][19][24] = 2;       // Far corner
    complex3D[7][10][12] = 3;        // Center-ish
    complex3D[14][0][24] = 4;        // Mixed corner
    complex3D[0][19][0] = 5;         // Another mixed corner

    print("Complex 3D access:");
    print("Origin [0][0][0] = " + complex3D[0][0][0]);
    print("Far corner [14][19][24] = " + complex3D[14][19][24]);
    print("Center [7][10][12] = " + complex3D[7][10][12]);
    print("Mixed corners: [14][0][24] = " + complex3D[14][0][24] + ", [0][19][0] = " + complex3D[0][19][0]);

    print("\nMulti-dimensional indexing edge cases tests completed");
}

testMultiDimensionalEdgeCases();