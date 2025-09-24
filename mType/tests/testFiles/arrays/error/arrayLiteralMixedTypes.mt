// Test mixed type validation in array literals
print("Testing mixed types in array literals");

// Mixed types should cause error
float[] values = [1.5, 2, true, "hello"];  // Error: multiple types

print("This should not be reached");