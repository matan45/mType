// Test that out of bounds access causes an error
print("Testing out of bounds access");

int[] arr = new int[3];
arr.set(0, 10);
arr.set(1, 20);
arr.set(2, 30);

// This should cause an error
int value = arr.get(5);

print("This should not be reached");