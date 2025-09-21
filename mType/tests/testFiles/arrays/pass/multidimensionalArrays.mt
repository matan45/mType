// Test multi-dimensional array support (improved syntax)
print("Testing multi-dimensional arrays");

// Test 2D array creation with bracket access and .length
print("Creating 2D array");
Array<Array<int>> matrix = new Array<Array<int>>();

// Initialize first row
Array<int> row1 = new Array<int>();
row1.add(1);
row1.add(2);
row1.add(3);
matrix.add(row1);

// Initialize second row
Array<int> row2 = new Array<int>();
row2.add(4);
row2.add(5);
row2.add(6);
matrix.add(row2);

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
Array<Array<string>> stringMatrix = new Array<Array<string>>();

Array<string> names1 = new Array<string>();
names1.add("Alice");
names1.add("Bob");
stringMatrix.add(names1);

Array<string> names2 = new Array<string>();
names2.add("Charlie");
names2.add("David");
stringMatrix.add(names2);

print("stringMatrix.length = " + stringMatrix.length);
print("stringMatrix[0][0] = " + stringMatrix[0][0]);
print("stringMatrix[1][1] = " + stringMatrix[1][1]);

print("Multi-dimensional arrays test completed");