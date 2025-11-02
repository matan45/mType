// Test memory reallocation during operations
print("Testing memory reallocation");

// Gradual growth pattern
int[] arr = new int[2];
arr[0] = 1;
arr[1] = 2;

print("Initial size 2:");
for (int i = 0; i < arr.length; i++) {
    print("  " + arr[i]);
}

// Grow to 4
int[] temp = new int[4];
for (int i = 0; i < arr.length; i++) {
    temp[i] = arr[i];
}
temp[2] = 3;
temp[3] = 4;
arr = temp;

print("After growing to 4:");
for (int i = 0; i < arr.length; i++) {
    print("  " + arr[i]);
}

// Grow to 8
int[] temp2 = new int[8];
for (int i = 0; i < arr.length; i++) {
    temp2[i] = arr[i];
}
for (int i = arr.length; i < temp2.length; i++) {
    temp2[i] = i + 1;
}
arr = temp2;

print("After growing to 8:");
for (int i = 0; i < arr.length; i++) {
    print("  " + arr[i]);
}

print("Memory reallocation test completed");
