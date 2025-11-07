// Test SIMD optimization with Int[] Box type arrays
// @Script
import * from "../../lib/primitives/Int.mt";

// Test 1: Small array (< 16, no SoA)
print("=== Small Int[] (size 5) ===");
Int[] small = new Int[5];
int i = 0;
while (i < 5) {
    small[i] = new Int(i + 1);
    i = i + 1;
}
print("small[0]: " + small[0].toString());
print("small[4]: " + small[4].toString());

// Test 2: Large array (>= 16, should use SoA + SIMD)
print("=== Large Int[] (size 20) ===");
Int[] large = new Int[20];
i = 0;
while (i < 20) {
    large[i] = new Int(i * 2);
    i = i + 1;
}
print("large[0]: " + large[0].toString());
print("large[19]: " + large[19].toString());

// Test 3: Manual sum with operator overloading
print("=== Sum with operator overloading ===");
Int sum = new Int(0);
i = 0;
while (i < 5) {
    sum = sum + small[i];  // Uses operator overloading
    i = i + 1;
}
print("Sum: " + sum.toString());

// Test 4: Sum using primitive values (direct field access on SoA)
print("=== Sum using primitive values ===");
int primitiveSum = 0;
i = 0;
while (i < 5) {
    primitiveSum = primitiveSum + small[i].getValue();
    i = i + 1;
}
print("Primitive sum: " + parsePrimitive(primitiveSum));
