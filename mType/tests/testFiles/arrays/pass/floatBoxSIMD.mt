// Test SIMD optimization with Float[] Box type arrays
// @Script
import * from "../../lib/primitives/Float.mt";

print("=== Float[] SIMD Test ===");

// Create large Float[] array (should trigger SoA + SIMD)
Float[] values = new Float[20];
int i = 0;
while (i < 20) {
    values[i] = new Float((float)i + 0.5);
    i = i + 1;
}

print("First element: " + values[0].toString());
print("Last element: " + values[19].toString());

// Sum using primitive values
float sum = 0.0;
i = 0;
while (i < 20) {
    sum = sum + values[i].getValue();
    i = i + 1;
}
print("Sum: " + parsePrimitive(sum));

// Test arithmetic with operator overloading
Float[] doubled = new Float[10];
i = 0;
while (i < 10) {
    doubled[i] = values[i] + values[i];  // Operator overloading
    i = i + 1;
}
print("doubled[0]: " + doubled[0].toString());
print("doubled[5]: " + doubled[5].toString());

print("Float[] SIMD test completed!");
