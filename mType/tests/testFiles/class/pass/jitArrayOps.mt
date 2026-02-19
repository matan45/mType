// JIT Test: Array operations (currently runs in interpreter due to JIT array bug)
// TODO: Add JIT warmup once array get/set/length JIT codegen is fixed

function sumArray(int[] arr): int {
    int sum = 0;
    for (int i = 0; i < arr.length; i = i + 1) {
        sum = sum + arr[i];
    }
    return sum;
}

function fillSequence(int[] arr): void {
    for (int i = 0; i < arr.length; i = i + 1) {
        arr[i] = i * 3;
    }
}

function bubbleSort(int[] arr): void {
    int n = arr.length;
    for (int i = 0; i < n - 1; i = i + 1) {
        for (int j = 0; j < n - i - 1; j = j + 1) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

function dotProduct(int[] a, int[] b): int {
    int sum = 0;
    for (int i = 0; i < a.length; i = i + 1) {
        sum = sum + a[i] * b[i];
    }
    return sum;
}

function findMax(int[] arr): int {
    int max = arr[0];
    for (int i = 1; i < arr.length; i = i + 1) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

// Array sum
int[] big = new int[1000];
fillSequence(big);
int sumTotal = 0;
for (int round = 0; round < 5000; round = round + 1) {
    sumTotal = sumTotal + sumArray(big);
}
print("5000x sumArray(1000): " + sumTotal);

// Dot product
int[] vecA = new int[500];
int[] vecB = new int[500];
for (int i = 0; i < 500; i = i + 1) {
    vecA[i] = i;
    vecB[i] = 500 - i;
}
int dpTotal = 0;
for (int round = 0; round < 10000; round = round + 1) {
    dpTotal = dpTotal + dotProduct(vecA, vecB);
}
print("10000x dotProduct(500): " + dpTotal);

// Find max
int maxTotal = 0;
for (int round = 0; round < 10000; round = round + 1) {
    maxTotal = maxTotal + findMax(big);
}
print("10000x findMax(1000): " + maxTotal);

// Bubble sort
int[] sortMe = new int[200];
for (int i = 0; i < 200; i = i + 1) {
    sortMe[i] = 200 - i;
}
for (int round = 0; round < 100; round = round + 1) {
    for (int i = 0; i < 200; i = i + 1) {
        sortMe[i] = 200 - i;
    }
    bubbleSort(sortMe);
}
print("sorted first: " + sortMe[0]);
print("sorted last: " + sortMe[199]);
