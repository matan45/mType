// Test accessing uninitialized array (should error)
print("Testing uninitialized array access");

// Declare array variable but don't initialize
int[] uninitializedArray;

// Attempting to access length should cause error
print("Length: " + uninitializedArray.length);

print("This should not be reached");
