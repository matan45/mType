// Test SIMD-optimized primitive arrays (int, float, bool)
// Arrays with size >= 16 automatically use SIMD storage (SSE2/AVX2/NEON)
// This provides 3-60x performance improvement for operations

// ============================================
// 1D SIMD Arrays - Int, Float, Bool
// ============================================

// Test 1.1: SIMD Int Array (1D) - Threshold optimization (>= 16 elements)
int[] simdInts = new int[20];
for (int i = 0; i < 20; i = i + 1) {
    simdInts[i] = i * 10;
}

// Verify SIMD int array operations
int sum = 0;
for (int i = 0; i < 20; i = i + 1) {
    sum = sum + simdInts[i];
}
print("SIMD Int Array Sum: " + sum); // Expected: 1900

// Test 1.2: SIMD Float Array (1D)
float[] simdFloats = new float[25];
for (int i = 0; i < 25; i = i + 1) {
    simdFloats[i] = i * 2.5;
}

float floatSum = 0.0;
for (int i = 0; i < 25; i = i + 1) {
    floatSum = floatSum + simdFloats[i];
}
print("SIMD Float Array Sum: " + floatSum); // Expected: 750.0

// Test 1.3: SIMD Bool Array (1D)
bool[] simdBools = new bool[30];
for (int i = 0; i < 30; i = i + 1) {
    simdBools[i] = (i % 2 == 0);
}

int trueCount = 0;
for (int i = 0; i < 30; i = i + 1) {
    if (simdBools[i]) {
        trueCount = trueCount + 1;
    }
}
print("SIMD Bool Array True Count: " + trueCount); // Expected: 15

// Test 1.4: Small array (< 16 elements) - Falls back to heterogeneous storage
int[] smallArray = new int[10];
for (int i = 0; i < 10; i = i + 1) {
    smallArray[i] = i;
}
print("Small Array [5]: " + smallArray[5]); // Expected: 5

// ============================================
// 2D SIMD Arrays - Multi-dimensional with flat storage
// ============================================

// Test 2.1: SIMD Int Array (2D) - 4x5 = 20 elements (>= 16, uses SIMD)
int[][] simdInts2D = new int[4][5];
for (int i = 0; i < 4; i = i + 1) {
    for (int j = 0; j < 5; j = j + 1) {
        simdInts2D[i][j] = i * 5 + j;
    }
}

// Verify 2D SIMD array access
print("SIMD 2D Int [2][3]: " + simdInts2D[2][3]); // Expected: 13

// Test 2.2: SIMD Float Array (2D) - 5x6 = 30 elements
float[][] simdFloats2D = new float[5][6];
for (int i = 0; i < 5; i = i + 1) {
    for (int j = 0; j < 6; j = j + 1) {
        simdFloats2D[i][j] = i * 1.5 + j * 0.5;
    }
}

print("SIMD 2D Float [3][4]: " + simdFloats2D[3][4]); // Expected: 6.5

// Test 2.3: SIMD Bool Array (2D) - 6x4 = 24 elements
bool[][] simdBools2D = new bool[6][4];
for (int i = 0; i < 6; i = i + 1) {
    for (int j = 0; j < 4; j = j + 1) {
        simdBools2D[i][j] = ((i + j) % 2 == 0);
    }
}

int true2DCount = 0;
for (int i = 0; i < 6; i = i + 1) {
    for (int j = 0; j < 4; j = j + 1) {
        if (simdBools2D[i][j]) {
            true2DCount = true2DCount + 1;
        }
    }
}
print("SIMD 2D Bool True Count: " + true2DCount); // Expected: 12

// ============================================
// 3D SIMD Arrays - High-dimensional optimization
// ============================================

// Test 3.1: SIMD Int Array (3D) - 3x3x3 = 27 elements (>= 16, uses SIMD)
int[][][] simdInts3D = new int[3][3][3];
for (int i = 0; i < 3; i = i + 1) {
    for (int j = 0; j < 3; j = j + 1) {
        for (int k = 0; k < 3; k = k + 1) {
            simdInts3D[i][j][k] = i * 9 + j * 3 + k;
        }
    }
}

print("SIMD 3D Int [1][2][1]: " + simdInts3D[1][2][1]); // Expected: 16

// Test 3.2: SIMD Float Array (3D) - 2x4x3 = 24 elements
float[][][] simdFloats3D = new float[2][4][3];
for (int i = 0; i < 2; i = i + 1) {
    for (int j = 0; j < 4; j = j + 1) {
        for (int k = 0; k < 3; k = k + 1) {
            simdFloats3D[i][j][k] = i * 12.0 + j * 3.0 + k * 1.0;
        }
    }
}

print("SIMD 3D Float [1][2][2]: " + simdFloats3D[1][2][2]); // Expected: 20.0

// Test 3.3: SIMD Bool Array (3D) - 4x4x2 = 32 elements
bool[][][] simdBools3D = new bool[4][4][2];
for (int i = 0; i < 4; i = i + 1) {
    for (int j = 0; j < 4; j = j + 1) {
        for (int k = 0; k < 2; k = k + 1) {
            simdBools3D[i][j][k] = (i == j);
        }
    }
}

int true3DCount = 0;
for (int i = 0; i < 4; i = i + 1) {
    for (int j = 0; j < 4; j = j + 1) {
        for (int k = 0; k < 2; k = k + 1) {
            if (simdBools3D[i][j][k]) {
                true3DCount = true3DCount + 1;
            }
        }
    }
}
print("SIMD 3D Bool True Count: " + true3DCount); // Expected: 8

// ============================================
// Large Arrays - Maximum SIMD Benefit
// ============================================

// Test 4.1: Large SIMD Int Array - 100 elements
int[] largeInts = new int[100];
for (int i = 0; i < 100; i = i + 1) {
    largeInts[i] = i * i;
}

print("Large SIMD Int [50]: " + largeInts[50]); // Expected: 2500

// Test 4.2: Large SIMD Float Array - 200 elements
float[] largeFloats = new float[200];
for (int i = 0; i < 200; i = i + 1) {
    largeFloats[i] = i * 0.5;
}

print("Large SIMD Float [100]: " + largeFloats[100]); // Expected: 50.0

print("SIMD Primitive Arrays Test Completed!");
