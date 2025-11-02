// Test binary search in sorted arrays
print("Testing binary search");

int[] sorted = new int[10];
for (int i = 0; i < sorted.length; i++) {
    sorted[i] = i * 10;
}

function binarySearch(int[] arr, int target): int {
    int left = 0;
    int right = arr.length - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        if (arr[mid] == target) {
            return mid;
        }

        if (arr[mid] < target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return -1;
}

print("Sorted array: [0, 10, 20, 30, 40, 50, 60, 70, 80, 90]");
print("binarySearch(40) = " + binarySearch(sorted, 40));
print("binarySearch(0) = " + binarySearch(sorted, 0));
print("binarySearch(90) = " + binarySearch(sorted, 90));
print("binarySearch(25) = " + binarySearch(sorted, 25));
print("binarySearch(100) = " + binarySearch(sorted, 100));

// Test with single element
int[] single = new int[1];
single[0] = 42;
print("Single element array:");
print("binarySearch(42) = " + binarySearch(single, 42));
print("binarySearch(10) = " + binarySearch(single, 10));

print("Binary search test completed");
