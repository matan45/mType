// Test type mismatch in array operations
print("Testing type mismatch");

// Create a valid array first
int[] numbers = new int[3];
numbers.set(0, 10);

// Try to assign a string to an int array element - this should cause an error
numbers.set(1, "invalid");

print("This should not be reached");