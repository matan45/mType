// Test nested iteration patterns
print("Testing nested iteration");

int[][] matrix = new int[3][];
matrix[0] = new int[3];
matrix[1] = new int[3];
matrix[2] = new int[3];

// Initialize matrix
int value = 1;
for (int i = 0; i < matrix.length; i++) {
    for (int j = 0; j < matrix[i].length; j++) {
        matrix[i][j] = value;
        value = value + 1;
    }
}

print("Matrix values:");
for (int i = 0; i < matrix.length; i++) {
    for (int j = 0; j < matrix[i].length; j++) {
        print("  matrix[" + i + "][" + j + "] = " + matrix[i][j]);
    }
}

// Triple nested iteration
int[][][] cube = new int[2][][];
for (int i = 0; i < cube.length; i++) {
    cube[i] = new int[2][];
    for (int j = 0; j < cube[i].length; j++) {
        cube[i][j] = new int[2];
        for (int k = 0; k < cube[i][j].length; k++) {
            cube[i][j][k] = i + j + k;
        }
    }
}

print("Cube values:");
for (int i = 0; i < cube.length; i++) {
    for (int j = 0; j < cube[i].length; j++) {
        for (int k = 0; k < cube[i][j].length; k++) {
            print("  cube[" + i + "][" + j + "][" + k + "] = " + cube[i][j][k]);
        }
    }
}

print("Nested iteration test completed");
