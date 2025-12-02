// Test array allocation with negative size
print("Testing array size validation");

// Negative array size should cause an error
int negativeSize = -1;

// This should cause an error due to negative size
int[] badArray = new int[negativeSize];

print("This should not be reached");
