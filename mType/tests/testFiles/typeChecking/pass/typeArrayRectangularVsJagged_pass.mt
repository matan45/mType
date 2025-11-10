// Test rectangular vs jagged array type checking

print("Testing rectangular vs jagged array types");

// Test rectangular array - all dimensions specified
print("Creating rectangular 2D array");
int[][] rectangular = new int[3][4];
rectangular[0][0] = 1;
rectangular[0][3] = 2;
rectangular[2][3] = 3;

print("Rectangular[0][0]: " + rectangular[0][0]);
print("Rectangular[0][3]: " + rectangular[0][3]);
print("Rectangular[2][3]: " + rectangular[2][3]);

// Test jagged array - variable-length sub-arrays
print("Creating jagged array");
string[][] jagged = new string[3][];
jagged[0] = new string[2];
jagged[1] = new string[4];
jagged[2] = new string[1];

jagged[0][0] = "A";
jagged[0][1] = "B";
jagged[1][0] = "C";
jagged[1][1] = "D";
jagged[1][2] = "E";
jagged[1][3] = "F";
jagged[2][0] = "G";

print("Jagged[0] length: " + jagged[0].length);
print("Jagged[1] length: " + jagged[1].length);
print("Jagged[2] length: " + jagged[2].length);
print("Jagged[0][0]: " + jagged[0][0]);
print("Jagged[1][3]: " + jagged[1][3]);
print("Jagged[2][0]: " + jagged[2][0]);

// Test 3D jagged array
print("Creating 3D jagged array");
int[][][] jagged3D = new int[2][][];
jagged3D[0] = new int[2][];
jagged3D[0][0] = new int[3];
jagged3D[0][1] = new int[2];
jagged3D[1] = new int[1][];
jagged3D[1][0] = new int[4];

jagged3D[0][0][0] = 100;
jagged3D[0][0][2] = 101;
jagged3D[0][1][1] = 102;
jagged3D[1][0][3] = 103;

print("Jagged3D[0][0][0]: " + jagged3D[0][0][0]);
print("Jagged3D[0][0][2]: " + jagged3D[0][0][2]);
print("Jagged3D[0][1][1]: " + jagged3D[0][1][1]);
print("Jagged3D[1][0][3]: " + jagged3D[1][0][3]);

print("Rectangular vs jagged array type checking passed");
