// Test array literals in multi-dimensional arrays
print("Testing array literals with multi-dimensional arrays");

// 2D arrays using array literals to initialize sub-arrays
print("Creating 2D array with array literals");
int[][] matrix = new int[2][];

// Initialize rows using array literals
matrix[0] = [1, 2, 3];
matrix[1] = [4, 5, 6];

print("2D array created with array literals");
print("matrix.length = " + matrix.length);
print("matrix[0].length = " + matrix[0].length);
print("matrix[1].length = " + matrix[1].length);

// Access elements
print("matrix[0][0] = " + matrix[0][0]);
print("matrix[0][2] = " + matrix[0][2]);
print("matrix[1][1] = " + matrix[1][1]);
print("matrix[1][2] = " + matrix[1][2]);

// Test with different sized rows (jagged arrays)
print("Creating jagged 2D array");
int[][] jaggedMatrix = new int[3][];
jaggedMatrix[0] = [1, 2];
jaggedMatrix[1] = [3, 4, 5, 6];
jaggedMatrix[2] = [7];

print("Jagged array lengths:");
for (int i = 0; i < jaggedMatrix.length; i++) {
    print("jaggedMatrix[" + i + "].length = " + jaggedMatrix[i].length);
    for (int j = 0; j < jaggedMatrix[i].length; j++) {
        print("jaggedMatrix[" + i + "][" + j + "] = " + jaggedMatrix[i][j]);
    }
}

// String multi-dimensional arrays with literals
print("Creating string 2D array with literals");
string[][] names = new string[2][];
names[0] = ["Alice", "Bob"];
names[1] = ["Charlie", "David", "Eve"];

for (int i = 0; i < names.length; i++) {
    for (int j = 0; j < names[i].length; j++) {
        print("names[" + i + "][" + j + "] = " + names[i][j]);
    }
}

// 3D arrays using array literals
print("Creating 3D array with array literals");
int[][][] cube = new int[2][][];
cube[0] = new int[2][];
cube[1] = new int[2][];

cube[0][0] = [1, 2];
cube[0][1] = [3, 4];
cube[1][0] = [5, 6];
cube[1][1] = [7, 8];

print("3D array access:");
print("cube[0][0][0] = " + cube[0][0][0]);
print("cube[0][0][1] = " + cube[0][0][1]);
print("cube[1][1][0] = " + cube[1][1][0]);
print("cube[1][1][1] = " + cube[1][1][1]);

print("Multi-dimensional array literals test completed");