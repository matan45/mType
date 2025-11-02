// Test operations on zero-length arrays
print("Testing zero-length arrays");

// Create zero-length arrays of different types
int[] emptyInts = new int[0];
print("Created zero-length int array, length: " + emptyInts.length);

string[] emptyStrings = new string[0];
print("Created zero-length string array, length: " + emptyStrings.length);

float[] emptyFloats = new float[0];
print("Created zero-length float array, length: " + emptyFloats.length);

// Test loop over zero-length array
int count = 0;
for (int i = 0; i < emptyInts.length; i++) {
    count = count + 1;
}
print("Loop iterations over zero-length array: " + count);

// Test comparisons
bool areEqual = emptyInts.length == 0;
print("emptyInts.length == 0: " + areEqual);

bool isGreater = emptyInts.length > 0;
print("emptyInts.length > 0: " + isGreater);

// Test array of zero-length arrays
int[][] arrayOfEmpty = new int[3][];
for (int i = 0; i < arrayOfEmpty.length; i++) {
    arrayOfEmpty[i] = new int[0];
}
print("Created array of 3 zero-length arrays");

for (int i = 0; i < arrayOfEmpty.length; i++) {
    print("arrayOfEmpty[" + i + "].length = " + arrayOfEmpty[i].length);
}

print("Zero-length arrays test completed");
