// Test null mixed with primitive types in array literals
print("Testing null with primitive types");

// Null with primitives should cause validation error
int[] numbers = [1, 2, null, 4];  // Error: null in primitive array

print("This should not be reached");