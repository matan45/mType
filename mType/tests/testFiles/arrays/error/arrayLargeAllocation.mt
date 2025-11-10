// Test array allocation with integer overflow calculation
print("Testing array size overflow calculation");

// Calculate a size that causes integer overflow
int largeSize = 2147483647; // INT_MAX
int overflowSize = largeSize + largeSize; // This should overflow to negative

// This should cause an error due to negative result from overflow
int[] badArray = new int[overflowSize];

print("This should not be reached");
