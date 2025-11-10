// Test multidimensional array type checking
print("Testing multidimensional array type checking");

// Test 2D int array type checking
int[][] matrix2D = new int[2][3];
matrix2D[0][0] = 10;
matrix2D[0][1] = 20;
matrix2D[0][2] = 30;
matrix2D[1][0] = 40;
matrix2D[1][1] = 50;
matrix2D[1][2] = 60;

print("2D array element [0][0]: " + matrix2D[0][0]);
print("2D array element [1][2]: " + matrix2D[1][2]);

// Test 3D string array type checking
string[][][] cube = new string[2][2][2];
cube[0][0][0] = "A";
cube[0][0][1] = "B";
cube[0][1][0] = "C";
cube[0][1][1] = "D";
cube[1][0][0] = "E";
cube[1][0][1] = "F";
cube[1][1][0] = "G";
cube[1][1][1] = "H";

print("3D array element [0][0][0]: " + cube[0][0][0]);
print("3D array element [1][1][1]: " + cube[1][1][1]);

// Test type checking with assignment
int[][] matrix2 = matrix2D;
print("Assignment works: " + matrix2[0][1]);

print("Multidimensional array type checking passed");
