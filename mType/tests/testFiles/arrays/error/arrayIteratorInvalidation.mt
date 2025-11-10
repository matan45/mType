// Test iterator invalidation scenario
print("Testing iterator invalidation");

int[] arr = new int[5];
for (int i = 0; i < arr.length; i++) {
    arr[i] = i * 10;
}

int savedLength = arr.length;

// Attempt to use saved length after potential modification
arr = new int[3];

// This will cause an error - accessing with old length
for (int i = 0; i < savedLength; i++) {
    print("arr[" + i + "] = " + arr[i]);
}

print("This should not be reached");
