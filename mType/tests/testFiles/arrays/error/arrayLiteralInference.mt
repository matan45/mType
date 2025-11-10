// Test type inference failure in array literals
print("Testing type inference failure");

// Mixed types in literal (should error if strict typing)
int[] mixed = [1, 2, "three", 4, 5];

print("This should not be reached");
