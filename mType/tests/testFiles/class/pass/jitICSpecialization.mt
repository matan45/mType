// JIT Test: Inline cache specialization and type feedback
// Tests that the IC type feedback mechanism correctly identifies
// operation types and the JIT produces correct specialized code

// Test 1: Integer arithmetic through IC path
// These will be recorded by TypeFeedbackCollector as INT operations
function intAdd(int a, int b): int {
    return a + b;
}

function intMul(int a, int b): int {
    return a * b;
}

function intSub(int a, int b): int {
    return a - b;
}

// Test 2: Float operations that must NOT be treated as int
// If the JIT incorrectly defaults to int, these will truncate
function preciseDiv(float a, float b): float {
    return a / b;
}

function preciseAdd(float a, float b): float {
    return a + b;
}

// Test 3: Computation where int truncation would give wrong results
function computePi(int terms): float {
    float pi = 0.0;
    for (int k = 0; k < terms; k = k + 1) {
        float term = 1.0 / (2.0 * k + 1.0);
        if (k % 2 == 0) {
            pi = pi + term;
        } else {
            pi = pi - term;
        }
    }
    return pi * 4.0;
}

// Test 4: Float accumulation where precision matters
function geometricSeries(float ratio, int n): float {
    float sum = 0.0;
    float term = 1.0;
    for (int i = 0; i < n; i = i + 1) {
        sum = sum + term;
        term = term * ratio;
    }
    return sum;
}

// Test 5: Mixed operations in a single function
function mixedCompute(int n): float {
    int intSum = 0;
    float floatSum = 0.0;
    for (int i = 1; i < n; i = i + 1) {
        intSum = intSum + i;
        float f = i * 1.0;
        floatSum = floatSum + 1.0 / f;
    }
    return floatSum;
}

// Warm up JIT
for (int i = 0; i < 150; i = i + 1) {
    intAdd(i, i);
    intMul(i, 2);
    intSub(i, 1);
    preciseDiv(1.0, 3.0);
    preciseAdd(0.1, 0.2);
    computePi(10);
    geometricSeries(0.5, 10);
    mixedCompute(10);
}

// === Verify JIT correctness ===

// Integer operations should produce exact results
print("intAdd(123456, 789012): " + intAdd(123456, 789012));
print("intMul(12345, 6789): " + intMul(12345, 6789));
print("intSub(1000000, 999999): " + intSub(1000000, 999999));

// Float operations should preserve precision
print("preciseDiv(1.0, 3.0): " + preciseDiv(1.0, 3.0));
print("preciseDiv(22.0, 7.0): " + preciseDiv(22.0, 7.0));
print("preciseAdd(0.1, 0.2): " + preciseAdd(0.1, 0.2));

// Pi approximation (needs float precision)
print("computePi(1000): " + computePi(1000));
print("computePi(10000): " + computePi(10000));

// Geometric series: 1 + 0.5 + 0.25 + ... converges to 2.0
print("geometricSeries(0.5, 50): " + geometricSeries(0.5, 50));

// Harmonic-like series (needs float division)
print("mixedCompute(100): " + mixedCompute(100));

// Stress test: many JIT-compiled calls
int intTotal = 0;
for (int i = 0; i < 10000; i = i + 1) {
    intTotal = intTotal + intAdd(1, 1);
}
print("10000x intAdd(1,1): " + intTotal);

float floatTotal = 0.0;
for (int i = 0; i < 10000; i = i + 1) {
    floatTotal = floatTotal + preciseDiv(1.0, 3.0);
}
print("10000x preciseDiv(1.0,3.0): " + floatTotal);
