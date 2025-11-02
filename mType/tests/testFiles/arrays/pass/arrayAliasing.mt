// Test array aliasing effects
print("Testing array aliasing");

int[] original = new int[4];
original[0] = 10;
original[1] = 20;
original[2] = 30;
original[3] = 40;

// Create alias (same reference)
int[] alias = original;

print("Original array:");
for (int i = 0; i < original.length; i++) {
    print("  original[" + i + "] = " + original[i]);
}

// Modify through alias
alias[0] = 100;
alias[2] = 300;

print("After modifying through alias:");
print("Original[0] = " + original[0]);
print("Alias[0] = " + alias[0]);
print("Original[2] = " + original[2]);
print("Alias[2] = " + alias[2]);

// Check if they reference the same array
bool sameRef = (original == alias);
print("Same reference: " + sameRef);

print("Array aliasing test completed");
