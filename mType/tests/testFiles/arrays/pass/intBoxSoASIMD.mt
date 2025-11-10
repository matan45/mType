// Test Structure-of-Arrays (SoA) optimization with SIMD for Int[] arrays
// @Script
import * from "../../lib/primitives/Int.mt";

// Create large Int[] array (should trigger SoA)
print("Creating Int[100] array (SoA + SIMD)...");
Int[] numbers = new Int[100];

// Fill with values
int i = 0;
while (i < 100) {
    numbers[i] = new Int(i + 1);
    i = i + 1;
}

// Test element access
print("Testing element access:");
print("numbers[0]: " + numbers[0].toString());
print("numbers[50]: " + numbers[50].toString());
print("numbers[99]: " + numbers[99].toString());

// Sum using primitive extraction (benefits from SIMD on SoA storage)
print("Computing sum...");
int sum = 0;
i = 0;
while (i < 100) {
    sum = sum + numbers[i].getValue();  // Direct field access on SoA
    i = i + 1;
}
print("Sum of all 100 elements: " + parsePrimitive(sum));
print("Expected: 5050");

// Test with arithmetic operations
print("Testing arithmetic operations:");
Int[] results = new Int[10];
i = 0;
while (i < 10) {
    Int a = numbers[i];
    Int b = numbers[i + 10];
    results[i] = a + b;  // Operator overloading
    i = i + 1;
}
print("results[0] (1+11): " + results[0].toString());
print("results[9] (10+20): " + results[9].toString());

print("Int[] SoA + SIMD test completed!");
