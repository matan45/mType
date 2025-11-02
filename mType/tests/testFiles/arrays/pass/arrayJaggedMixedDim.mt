// Test inconsistent dimension depths in jagged arrays
print("Testing jagged arrays with mixed dimensions");

int[][] jagged1 = new int[4][];
jagged1[0] = new int[1];
jagged1[1] = new int[5];
jagged1[2] = new int[2];
jagged1[3] = new int[3];

// Fill with values
int value = 0;
for (int i = 0; i < jagged1.length; i++) {
    for (int j = 0; j < jagged1[i].length; j++) {
        jagged1[i][j] = value;
        value = value + 1;
    }
}

print("Jagged array with different row lengths:");
for (int i = 0; i < jagged1.length; i++) {
    print("  Row " + i + " has " + jagged1[i].length + " elements");
}

// 3D jagged with very mixed dimensions
int[][][] jagged3D = new int[2][][];
jagged3D[0] = new int[3][];
jagged3D[0][0] = new int[2];
jagged3D[0][1] = new int[1];
jagged3D[0][2] = new int[4];

jagged3D[1] = new int[1][];
jagged3D[1][0] = new int[5];

print("3D jagged dimensions:");
for (int i = 0; i < jagged3D.length; i++) {
    for (int j = 0; j < jagged3D[i].length; j++) {
        print("  jagged3D[" + i + "][" + j + "] has " + jagged3D[i][j].length + " elements");
    }
}

print("Jagged mixed dimensions test completed");
