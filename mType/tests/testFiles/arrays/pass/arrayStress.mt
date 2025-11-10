// Test large arrays (10000+ elements) for stress testing
print("Testing large array stress");

int size = 10000;
int[] large = new int[size];

print("Initializing array of " + size + " elements");

// Initialize
for (int i = 0; i < large.length; i++) {
    large[i] = i % 100;
}

// Calculate sum
int sum = 0;
for (int i = 0; i < large.length; i++) {
    sum = sum + large[i];
}

print("Sum of " + size + " elements: " + sum);

// Find max
int max = large[0];
for (int i = 1; i < large.length; i++) {
    if (large[i] > max) {
        max = large[i];
    }
}

print("Max value: " + max);

// Count specific value
int target = 42;
int count = 0;
for (int i = 0; i < large.length; i++) {
    if (large[i] == target) {
        count = count + 1;
    }
}

print("Count of " + target + ": " + count);

// Reverse array
for (int i = 0; i < large.length / 2; i++) {
    int temp = large[i];
    large[i] = large[large.length - 1 - i];
    large[large.length - 1 - i] = temp;
}

print("Array reversed, first element: " + large[0]);
print("Array reversed, last element: " + large[large.length - 1]);

print("Stress test completed");
