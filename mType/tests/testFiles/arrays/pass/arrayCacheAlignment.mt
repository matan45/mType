// Test cache line effects with different array sizes
print("Testing cache alignment effects");

// Multiple of cache line (typically 64 bytes = 16 ints)
int[] aligned = new int[16];
for (int i = 0; i < aligned.length; i++) {
    aligned[i] = i;
}

print("Cache-aligned array (16 elements):");
for (int i = 0; i < aligned.length; i++) {
    print("  aligned[" + i + "] = " + aligned[i]);
}

// Non-aligned sizes
int[] unaligned1 = new int[13];
int[] unaligned2 = new int[19];
int[] unaligned3 = new int[21];

for (int i = 0; i < unaligned1.length; i++) {
    unaligned1[i] = i * 2;
}

print("Unaligned array (13 elements) sum:");
int sum = 0;
for (int i = 0; i < unaligned1.length; i++) {
    sum = sum + unaligned1[i];
}
print("Sum: " + sum);

print("Cache alignment test completed");
