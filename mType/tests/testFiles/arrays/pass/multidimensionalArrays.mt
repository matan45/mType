// Test multi-dimensional array support using built-in syntax
print("Testing multi-dimensional arrays");

// Test 2D array creation using built-in syntax
print("Creating 2D array");
int[][] matrix = new int[2][3];

// Initialize elements
matrix[0][0] = 1;
matrix[0][1] = 2;
matrix[0][2] = 3;
matrix[1][0] = 4;
matrix[1][1] = 5;
matrix[1][2] = 6;

print("2D array created");
print("matrix.length = " + matrix.length);
print("matrix[0].length = " + matrix[0].length);
print("matrix[1].length = " + matrix[1].length);

// Access elements using bracket syntax
print("matrix[0][0] = " + matrix[0][0]);
print("matrix[0][1] = " + matrix[0][1]);
print("matrix[1][0] = " + matrix[1][0]);
print("matrix[1][2] = " + matrix[1][2]);

// Test nested arrays with string
print("Creating string nested array");
string[][] stringMatrix = new string[2][2];

stringMatrix[0][0] = "Alice";
stringMatrix[0][1] = "Bob";
stringMatrix[1][0] = "Charlie";
stringMatrix[1][1] = "David";

print("stringMatrix.length = " + stringMatrix.length);
print("stringMatrix[0][0] = " + stringMatrix[0][0]);
print("stringMatrix[1][1] = " + stringMatrix[1][1]);

print("Multi-dimensional arrays test completed");