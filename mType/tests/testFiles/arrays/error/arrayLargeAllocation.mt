// Test very large allocation failure
print("Testing large allocation");

// Attempt to allocate extremely large array (may cause error)
int[] huge = new int[2147483647];

print("This should not be reached");
