// Test sorting with custom comparators (bubble sort implementation)
print("Testing array sorting");

int[] unsorted = new int[8];
unsorted[0] = 64;
unsorted[1] = 34;
unsorted[2] = 25;
unsorted[3] = 12;
unsorted[4] = 22;
unsorted[5] = 11;
unsorted[6] = 90;
unsorted[7] = 88;

print("Unsorted array:");
for (int i = 0; i < unsorted.length; i++) {
    print("  " + unsorted[i]);
}

// Bubble sort ascending
function bubbleSortAscending(int[] arr): void {
    for (int i = 0; i < arr.length - 1; i++) {
        for (int j = 0; j < arr.length - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

bubbleSortAscending(unsorted);

print("Sorted ascending:");
for (int i = 0; i < unsorted.length; i++) {
    print("  " + unsorted[i]);
}

// Bubble sort descending
int[] descending = new int[5];
descending[0] = 5;
descending[1] = 2;
descending[2] = 8;
descending[3] = 1;
descending[4] = 9;

function bubbleSortDescending(int[] arr): void {
    for (int i = 0; i < arr.length - 1; i++) {
        for (int j = 0; j < arr.length - i - 1; j++) {
            if (arr[j] < arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

bubbleSortDescending(descending);

print("Sorted descending:");
for (int i = 0; i < descending.length; i++) {
    print("  " + descending[i]);
}

print("Sorting test completed");
