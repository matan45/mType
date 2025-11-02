// Test MIN_VALUE and MAX_VALUE integers in arrays
print("Testing integer boundary values in arrays");

int[] boundaries = new int[4];
print("Created int array with length: " + boundaries.length);

// Assign boundary values
boundaries[0] = 2147483647;   // MAX_VALUE (2^31 - 1)
boundaries[1] = -2147483648;  // MIN_VALUE (-2^31)
boundaries[2] = 0;
boundaries[3] = -1;

// Display values
for (int i = 0; i < boundaries.length; i++) {
    print("boundaries[" + i + "] = " + boundaries[i]);
}

// Test arithmetic near boundaries
print("Testing arithmetic near boundaries:");

// Test addition that would overflow (wraps around)
int maxPlusOne = boundaries[0] + 1;
print("MAX_VALUE + 1 = " + maxPlusOne);

// Test subtraction that would underflow
int minMinusOne = boundaries[1] - 1;
print("MIN_VALUE - 1 = " + minMinusOne);

// Test negation of MIN_VALUE
int negatedMin = -boundaries[1];
print("-(MIN_VALUE) = " + negatedMin);

// Store results in array
int[] results = new int[3];
results[0] = boundaries[0] - 1;  // MAX_VALUE - 1
results[1] = boundaries[1] + 1;  // MIN_VALUE + 1
results[2] = boundaries[0] / 2;  // Half of MAX_VALUE

print("Boundary calculations:");
for (int i = 0; i < results.length; i++) {
    print("results[" + i + "] = " + results[i]);
}

print("Integer boundary values test completed");
