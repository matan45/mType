// Test exact 16-element SIMD threshold
print("Testing SIMD threshold");

// Exactly 16 elements (SIMD threshold)
int[] simdArray = new int[16];
for (int i = 0; i < simdArray.length; i++) {
    simdArray[i] = i * 2;
}

print("SIMD array (16 elements):");
int sum16 = 0;
for (int i = 0; i < simdArray.length; i++) {
    sum16 = sum16 + simdArray[i];
}
print("Sum of 16 elements: " + sum16);

// Just below threshold (15 elements)
int[] belowSIMD = new int[15];
for (int i = 0; i < belowSIMD.length; i++) {
    belowSIMD[i] = i * 2;
}

int sum15 = 0;
for (int i = 0; i < belowSIMD.length; i++) {
    sum15 = sum15 + belowSIMD[i];
}
print("Sum of 15 elements: " + sum15);

// Just above threshold (17 elements)
int[] aboveSIMD = new int[17];
for (int i = 0; i < aboveSIMD.length; i++) {
    aboveSIMD[i] = i * 2;
}

int sum17 = 0;
for (int i = 0; i < aboveSIMD.length; i++) {
    sum17 = sum17 + aboveSIMD[i];
}
print("Sum of 17 elements: " + sum17);

print("SIMD threshold test completed");
