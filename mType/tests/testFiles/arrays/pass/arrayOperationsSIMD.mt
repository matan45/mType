// Test SIMD-accelerated array operations
// Note: Arrays must be >= 16 elements to use SIMD acceleration

// ========== Test Arithmetic Operations (int) ==========

function testIntArithmetic(): void {
    int[] a = new int[20];
    int[] b = new int[20];

    // Initialize arrays
    for (int i = 0; i < 20; i = i + 1) {
        a[i] = i + 1;
        b[i] = i + 2;
    }

    // Test arrayAdd
    int[] sum = arrayAdd(a, b);
    print("arrayAdd: ");
    print(sum[0]); // 3
    print(" ");
    print(sum[5]); // 13
    print(" ");
    print(sum[19]); // 41

    // Test arrayAddScalar
    int[] sumScalar = arrayAddScalar(a, 10);
    print(" arrayAddScalar: ");
    print(sumScalar[0]); // 11
    print(" ");
    print(sumScalar[9]); // 20

    // Test arraySubtract
    int[] diff = arraySubtract(b, a);
    print(" arraySubtract: ");
    print(diff[0]); // 1
    print(" ");
    print(diff[10]); // 1

    // Test arrayMultiply
    int[] product = arrayMultiply(a, b);
    print(" arrayMultiply: ");
    print(product[0]); // 2
    print(" ");
    print(product[4]); // 30

    // Test arrayMultiplyScalar
    int[] productScalar = arrayMultiplyScalar(a, 3);
    print(" arrayMultiplyScalar: ");
    print(productScalar[0]); // 3
    print(" ");
    print(productScalar[5]); // 18
}

// ========== Test Arithmetic Operations (float) ==========

function testFloatArithmetic(): void {
    float[] a = new float[16];
    float[] b = new float[16];

    // Initialize arrays
    for (int i = 0; i < 16; i = i + 1) {
        a[i] = 2.5;
        b[i] = 1.5;
    }

    // Test arrayAdd
    float[] sum = arrayAdd(a, b);
    print(" arrayAdd(float): ");
    print(sum[0]); // 4.0
    print(" ");
    print(sum[10]); // 4.0

    // Test arrayAddScalar
    float[] sumScalar = arrayAddScalar(a, 5.0);
    print(" arrayAddScalar(float): ");
    print(sumScalar[0]); // 7.5

    // Test arrayMultiply
    float[] product = arrayMultiply(a, b);
    print(" arrayMultiply(float): ");
    print(product[0]); // 3.75
}

// ========== Test Reduction Operations ==========

function testReductionOps(): void {
    int[] arr = new int[20];
    for (int i = 0; i < 20; i = i + 1) {
        arr[i] = i + 1; // [1, 2, 3, ..., 20]
    }

    // Test arraySum
    int total = arraySum(arr);
    print(" arraySum: ");
    print(total); // 210

    // Test arrayMin
    int minimum = arrayMin(arr);
    print(" arrayMin: ");
    print(minimum); // 1

    // Test arrayMax
    int maximum = arrayMax(arr);
    print(" arrayMax: ");
    print(maximum); // 20

    // Test arrayAverage
    float avg = arrayAverage(arr);
    print(" arrayAverage: ");
    print(avg); // 10.5

    // Float array reduction
    float[] farr = new float[16];
    for (int i = 0; i < 16; i = i + 1) {
        farr[i] = 2.5;
    }

    float fsum = arraySum(farr);
    print(" arraySum(float): ");
    print(fsum); // 40.0
}

// ========== Test Utility Operations ==========

function testUtilityOps() : void{
    // Test arrayFill
    int[] arr = new int[20];
    arrayFill(arr, 42);
    print(" arrayFill: ");
    print(arr[0]); // 42
    print(" ");
    print(arr[10]); // 42
    print(" ");
    print(arr[19]); // 42

    // Test arrayCopy
    int[] arr2 = new int[16];
    for (int i = 0; i < 16; i = i + 1) {
        arr2[i] = i + 1;
    }
    int[] copy = arrayCopy(arr2);
    copy[0] = 999;
    print(" arrayCopy: ");
    print(arr2[0]); // 1 (original unchanged)
    print(" ");
    print(copy[0]); // 999 (copy modified)

    // Test arrayReverse
    int[] arr3 = new int[16];
    for (int i = 0; i < 16; i = i + 1) {
        arr3[i] = i + 1;
    }
    arrayReverse(arr3);
    print(" arrayReverse: ");
    print(arr3[0]); // 16
    print(" ");
    print(arr3[15]); // 1
}

// ========== Test Small Arrays (Fallback) ==========

function testSmallArrays(): void {
    // Arrays < 16 elements use scalar fallback
    int[] small1 = new int[5];
    int[] small2 = new int[5];

    for (int i = 0; i < 5; i = i + 1) {
        small1[i] = i + 1;
        small2[i] = i + 2;
    }

    int[] sum = arrayAdd(small1, small2);
    print(" smallArrayAdd: ");
    print(sum[0]); // 3
    print(" ");
    print(sum[4]); // 11

    int total = arraySum(small1);
    print(" smallArraySum: ");
    print(total); // 15
}

// ========== Run All Tests ==========

testIntArithmetic();
testFloatArithmetic();
testReductionOps();
testUtilityOps();
testSmallArrays();
