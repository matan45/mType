// Test 4D jagged array creation and usage
print("Testing 4D jagged arrays");

// Create 4D jagged array
int[][][][] hyperCube = new int[2][][][];

print("4D array created with length: " + hyperCube.length);

// Initialize first level (3D arrays)
hyperCube[0] = new int[2][][];
hyperCube[1] = new int[2][][];

// Initialize second level (2D arrays)
hyperCube[0][0] = new int[2][];
hyperCube[0][1] = new int[2][];
hyperCube[1][0] = new int[2][];
hyperCube[1][1] = new int[2][];

// Initialize third level (1D arrays) with different sizes for each
hyperCube[0][0][0] = [1, 2];
hyperCube[0][0][1] = [3, 4, 5];
hyperCube[0][1][0] = [6];
hyperCube[0][1][1] = [7, 8, 9, 10];

hyperCube[1][0][0] = [11, 12, 13];
hyperCube[1][0][1] = [14, 15];
hyperCube[1][1][0] = [16, 17, 18, 19, 20];
hyperCube[1][1][1] = [21];

print("4D array populated with different sized sub-arrays");

// Access and print all elements
print("4D array contents:");
for (int i = 0; i < hyperCube.length; i++) {
    print("Dimension 1[" + i + "]:");
    for (int j = 0; j < hyperCube[i].length; j++) {
        print("  Dimension 2[" + j + "]:");
        for (int k = 0; k < hyperCube[i][j].length; k++) {
            print("    Dimension 3[" + k + "] length: " + hyperCube[i][j][k].length);
            for (int l = 0; l < hyperCube[i][j][k].length; l++) {
                print("      hyperCube[" + i + "][" + j + "][" + k + "][" + l + "] = " + hyperCube[i][j][k][l]);
            }
        }
    }
}

print("4D jagged array test completed");