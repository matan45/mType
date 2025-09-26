// Test enhanced n-dimensional array assignment for 3D+ arrays

print("Testing enhanced n-dimensional array assignment");

// Test 1: 3D FlatMultiArray assignment
print("\n=== 3D FlatMultiArray ===");
int[][][] flat3d = new int[2][2][2];
flat3d[0][0][0] = 111;
flat3d[1][1][1] = 222;
flat3d[0][1][0] = 333;
print("flat3d[0][0][0] = " + flat3d[0][0][0]);
print("flat3d[1][1][1] = " + flat3d[1][1][1]);
print("flat3d[0][1][0] = " + flat3d[0][1][0]);

// Test 2: 4D FlatMultiArray assignment
print("\n=== 4D FlatMultiArray ===");
int[][][][] flat4d = new int[2][2][2][2];
flat4d[0][0][0][1] = 444;
flat4d[1][1][1][0] = 555;
flat4d[0][1][0][1] = 666;
print("flat4d[0][0][0][1] = " + flat4d[0][0][0][1]);
print("flat4d[1][1][1][0] = " + flat4d[1][1][1][0]);
print("flat4d[0][1][0][1] = " + flat4d[0][1][0][1]);

// Test 3: 3D Jagged Array assignment
print("\n=== 3D Jagged Array ===");
int[][][] jagged3d = new int[2][][];

// Initialize first dimension
jagged3d[0] = new int[2][];
jagged3d[0][0] = new int[3];
jagged3d[0][1] = new int[2];

// Initialize second dimension
jagged3d[1] = new int[1][];
jagged3d[1][0] = new int[4];

// Test assignments using enhanced n-dimensional assignment
jagged3d[0][0][1] = 777;
jagged3d[0][1][0] = 888;
jagged3d[1][0][2] = 999;

print("jagged3d[0][0][1] = " + jagged3d[0][0][1]);
print("jagged3d[0][1][0] = " + jagged3d[0][1][0]);
print("jagged3d[1][0][2] = " + jagged3d[1][0][2]);

// Test 4: 5D FlatMultiArray assignment
print("\n=== 5D FlatMultiArray ===");
int[][][][][] flat5d = new int[2][2][2][2][2];
flat5d[0][1][0][1][0] = 1000;
flat5d[1][0][1][0][1] = 2000;
flat5d[0][0][1][1][0] = 3000;
print("flat5d[0][1][0][1][0] = " + flat5d[0][1][0][1][0]);
print("flat5d[1][0][1][0][1] = " + flat5d[1][0][1][0][1]);
print("flat5d[0][0][1][1][0] = " + flat5d[0][0][1][1][0]);

// Test 5: Mixed string and int 3D arrays
print("\n=== 3D String Array ===");
string[][][] str3d = new string[2][2][2];
str3d[0][0][1] = "Hello";
str3d[1][1][0] = "World";
str3d[0][1][1] = "NDimensional";
print("str3d[0][0][1] = " + str3d[0][0][1]);
print("str3d[1][1][0] = " + str3d[1][1][0]);
print("str3d[0][1][1] = " + str3d[0][1][1]);

print("\nAll n-dimensional array assignment tests completed successfully!");