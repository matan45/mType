// JIT Test: Type-feedback-guided arithmetic specialization
// When both operands are boxed, the JIT uses TypeFeedbackCollector
// to determine whether to unbox as INT or FLOAT

// Test 1: Float addition through repeated calls
// The TypeFeedbackCollector observes float operands and guides
// the JIT to use float arithmetic instead of defaulting to int
function floatSum(float a, float b): float {
    return a + b;
}

function floatProduct(float a, float b): float {
    return a * b;
}

function floatDifference(float a, float b): float {
    return a - b;
}

function floatDivision(float a, float b): float {
    return a / b;
}

// Test 2: Loop with float accumulation
// Type feedback should observe that the loop variable and accumulator
// are consistently float, guiding JIT to emit float instructions
function floatAccumulateLoop(int n): float {
    float sum = 0.0;
    float factor = 1.5;
    for (int i = 0; i < n; i = i + 1) {
        sum = sum + factor;
        factor = factor * 0.99;
    }
    return sum;
}

// Test 3: Mixed int-float promotion
// When one operand is int and the other is float, the result should be float
function mixedArithmetic(int count): float {
    float result = 0.0;
    for (int i = 1; i < count; i = i + 1) {
        float f = i * 1.0;
        result = result + f * 2.5;
    }
    return result;
}

// Test 4: Float division and modulo in hot loops
function floatDivLoop(int n): float {
    float result = 1000.0;
    for (int i = 1; i < n; i = i + 1) {
        float divisor = i * 1.0;
        result = result / (1.0 + 1.0 / divisor);
    }
    return result;
}

// Test 5: Chained float operations (add, sub, mul, div in one function)
function chainedFloatOps(float start, int iterations): float {
    float val = start;
    for (int i = 0; i < iterations; i = i + 1) {
        val = val + 1.5;
        val = val * 0.98;
        val = val - 0.3;
        if (val > 500.0) {
            val = val / 2.0;
        }
    }
    return val;
}

// === Warm up JIT (150+ calls to reach compilation threshold) ===
for (int i = 0; i < 150; i = i + 1) {
    floatSum(1.0, 2.0);
    floatProduct(1.0, 2.0);
    floatDifference(3.0, 1.0);
    floatDivision(6.0, 2.0);
    floatAccumulateLoop(10);
    mixedArithmetic(10);
    floatDivLoop(10);
    chainedFloatOps(100.0, 10);
}

// === Run tests with JIT-compiled functions ===

// Test basic float operations
print("floatSum(3.14, 2.86): " + floatSum(3.14, 2.86));
print("floatProduct(2.5, 4.0): " + floatProduct(2.5, 4.0));
print("floatDifference(10.5, 3.25): " + floatDifference(10.5, 3.25));
print("floatDivision(15.0, 4.0): " + floatDivision(15.0, 4.0));

// Test float accumulation loop
float accum = floatAccumulateLoop(1000);
print("floatAccumulateLoop(1000): " + accum);

// Test mixed arithmetic
float mixed = mixedArithmetic(100);
print("mixedArithmetic(100): " + mixed);

// Test float division loop
float divResult = floatDivLoop(100);
print("floatDivLoop(100): " + divResult);

// Test chained operations
float chained = chainedFloatOps(100.0, 500);
print("chainedFloatOps(100.0, 500): " + chained);

// Test repeated JIT-compiled calls for consistency
float total = 0.0;
for (int i = 0; i < 1000; i = i + 1) {
    total = total + floatSum(0.001, 0.001);
}
print("1000x floatSum(0.001, 0.001): " + total);

float productTotal = 1.0;
for (int i = 0; i < 100; i = i + 1) {
    productTotal = productTotal * floatProduct(1.0, 1.001);
}
print("100x compounding floatProduct(1.0, 1.001): " + productTotal);
