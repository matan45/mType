// Test nested literals with mixed depths
print("Testing mixed depth literals");

// 2D array with varying row lengths
int[][] jagged = [
    [1, 2, 3],
    [4, 5],
    [6, 7, 8, 9],
    [10]
];

print("Jagged array:");
for (int i = 0; i < jagged.length; i++) {
    print("  Row " + i + " (length " + jagged[i].length + "):");
    for (int j = 0; j < jagged[i].length; j++) {
        print("    jagged[" + i + "][" + j + "] = " + jagged[i][j]);
    }
}

// 3D array with varying depths
int[][][] complex = [
    [[1, 2], [3, 4, 5]],
    [[6], [7, 8], [9, 10, 11]]
];

print("Complex 3D array:");
for (int i = 0; i < complex.length; i++) {
    for (int j = 0; j < complex[i].length; j++) {
        for (int k = 0; k < complex[i][j].length; k++) {
            print("  complex[" + i + "][" + j + "][" + k + "] = " + complex[i][j][k]);
        }
    }
}

print("Mixed depth literals test completed");
