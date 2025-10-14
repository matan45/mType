// Comprehensive test for array operations covering edge cases and mixed scenarios

// Test 1: Large arrays (SIMD optimization)
function testLargeArrays(): void {
    print("Large arrays:");
    int[] large1 = new int[100];
    int[] large2 = new int[100];

    for (int i = 0; i < 100; i = i + 1) {
        large1[i] = i;
        large2[i] = 100 - i;
    }

    int[] sum = arrayAdd(large1, large2);
    print(" sum[0]=");
    print(sum[0]); // 100
    print(" sum[50]=");
    print(sum[50]); // 100

    int total = arraySum(large1);
    print(" sum=");
    print(total); // 4950

    int min = arrayMin(large1);
    int max = arrayMax(large1);
    print(" min=");
    print(min); // 0
    print(" max=");
    print(max); // 99
}

// Test 2: Chained operations
function testChainedOps() : void{
    print(" Chained:");
    int[] arr = new int[20];
    for (int i = 0; i < 20; i = i + 1) {
        arr[i] = i + 1;
    }

    // Chain: (arr + arr) * 2 + 5
    int[] step1 = arrayAdd(arr, arr);
    int[] step2 = arrayMultiplyScalar(step1, 2);
    int[] step3 = arrayAddScalar(step2, 5);

    print(" result[0]=");
    print(step3[0]); // (1+1)*2+5 = 9
    print(" result[5]=");
    print(step3[5]); // (6+6)*2+5 = 29
}

// Test 3: Float precision
function testFloatPrecision(): void {
    print(" Float:");
    float[] arr = new float[16];
    for (int i = 0; i < 16; i = i + 1) {
        arr[i] = 0.1;
    }

    float sum = arraySum(arr);
    print(" sum=");
    print(sum); // 1.6 (should be close)

    float avg = arrayAverage(arr);
    print(" avg=");
    print(avg); // 0.1
}

// Test 4: Array copy independence
function testCopyIndependence(): void {
    print(" Copy:");
    int[] original = new int[16];
    for (int i = 0; i < 16; i = i + 1) {
        original[i] = i;
    }

    int[] copy1 = arrayCopy(original);
    int[] copy2 = arrayCopy(original);

    copy1[0] = 999;
    copy2[0] = 888;

    print(" orig=");
    print(original[0]); // 0
    print(" c1=");
    print(copy1[0]); // 999
    print(" c2=");
    print(copy2[0]); // 888
}

// Test 5: Reverse and operations
function testReverseOps(): void {
    print(" Reverse:");
    int[] arr = new int[20];
    for (int i = 0; i < 20; i = i + 1) {
        arr[i] = i + 1;
    }

    int beforeSum = arraySum(arr);
    arrayReverse(arr);
    int afterSum = arraySum(arr);

    print(" before=");
    print(beforeSum); // 210
    print(" after=");
    print(afterSum); // 210 (same)
    print(" first=");
    print(arr[0]); // 20
    print(" last=");
    print(arr[19]); // 1
}

// Test 6: Fill and operations
function testFillOps(): void {
    print(" Fill:");
    int[] arr = new int[20];
    arrayFill(arr, 5);

    int sum = arraySum(arr);
    print(" sum=");
    print(sum); // 100

    int[] doubled = arrayMultiplyScalar(arr, 2);
    int sum2 = arraySum(doubled);
    print(" doubled=");
    print(sum2); // 200
}

// Test 7: Empty and single element (edge cases)
function testEdgeSizes(): void {
    print(" Edge:");

    // Single element
    int[] single = new int[1];
    single[0] = 42;
    int sum1 = arraySum(single);
    print(" single=");
    print(sum1); // 42

    // Small array (below SIMD threshold)
    int[] small = new int[5];
    for (int i = 0; i < 5; i = i + 1) {
        small[i] = i + 1;
    }
    int sum5 = arraySum(small);
    print(" small=");
    print(sum5); // 15
}

// Test 8: Arithmetic with different values
function testArithmeticValues(): void {
    print(" Values:");
    int[] arr = new int[16];

    // Powers of 2
    for (int i = 0; i < 16; i = i + 1) {
        arr[i] = 1;
    }
    int[] doubled = arrayMultiplyScalar(arr, 2);
    int[] quadrupled = arrayMultiply(doubled, doubled);

    print(" quad[0]=");
    print(quadrupled[0]); // 4

    // Negative values
    int[] neg = arrayMultiplyScalar(arr, -1);
    int negSum = arraySum(neg);
    print(" negSum=");
    print(negSum); // -16
}

// Test 9: Min/Max with various distributions
function testMinMaxDistributions(): void {
    print(" MinMax:");

    // Ascending
    int[] asc = new int[20];
    for (int i = 0; i < 20; i = i + 1) {
        asc[i] = i;
    }
    print(" ascMin=");
    print(arrayMin(asc)); // 0
    print(" ascMax=");
    print(arrayMax(asc)); // 19

    // Descending
    int[] desc = new int[20];
    for (int i = 0; i < 20; i = i + 1) {
        desc[i] = 19 - i;
    }
    print(" descMin=");
    print(arrayMin(desc)); // 0
    print(" descMax=");
    print(arrayMax(desc)); // 19

    // All same
    int[] same = new int[20];
    arrayFill(same, 7);
    print(" sameMin=");
    print(arrayMin(same)); // 7
    print(" sameMax=");
    print(arrayMax(same)); // 7
}

// Run all tests
testLargeArrays();
testChainedOps();
testFloatPrecision();
testCopyIndependence();
testReverseOps();
testFillOps();
testEdgeSizes();
testArithmeticValues();
testMinMaxDistributions();
print(" Done");
