// JIT Test: Float arithmetic, modulo, and unary operations

function floatAccumulate(int iterations): float {
    float sum = 0.0;
    for (int i = 1; i < iterations; i = i + 1) {
        float f = i * 1.0;
        sum = sum + f / (f + 1.0);
    }
    return sum;
}

function moduloSum(int n): int {
    int sum = 0;
    for (int i = 0; i < n; i = i + 1) {
        sum = sum + (i % 7);
    }
    return sum;
}

function floatMulDiv(int iterations): float {
    float result = 1.0;
    for (int i = 1; i < iterations; i = i + 1) {
        float f = i * 1.0;
        result = result * (1.0 + 1.0 / f);
        if (result > 1000.0) {
            result = result / 500.0;
        }
    }
    return result;
}

function floatSubLoop(int iterations): float {
    float val = 10000.0;
    for (int i = 0; i < iterations; i = i + 1) {
        val = val - 0.5;
        val = val + 0.3;
    }
    return val;
}

// Warm up JIT (100+ calls)
for (int i = 0; i < 150; i = i + 1) {
    floatAccumulate(10);
    moduloSum(10);
    floatMulDiv(10);
    floatSubLoop(10);
}

// Heavy float accumulation
float r1 = floatAccumulate(50000);
print("floatAccumulate(50000): " + r1);

// Heavy modulo
int r2 = moduloSum(500000);
print("moduloSum(500000): " + r2);

// Heavy mul/div
float r3 = floatMulDiv(100000);
print("floatMulDiv(100000): " + r3);

// Heavy sub loop
float r4 = floatSubLoop(200000);
print("floatSubLoop(200000): " + r4);

// Run each multiple times to amplify JIT benefit
float total = 0.0;
for (int round = 0; round < 20; round = round + 1) {
    total = total + floatAccumulate(10000);
}
print("20x floatAccumulate(10000): " + total);

int modTotal = 0;
for (int round = 0; round < 50; round = round + 1) {
    modTotal = modTotal + moduloSum(100000);
}
print("50x moduloSum(100000): " + modTotal);
