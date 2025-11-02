// Test computed index bounds checking
print("Testing computed indices");

int[] arr = new int[10];
for (int i = 0; i < arr.length; i++) {
    arr[i] = i * 5;
}

// Test various computed index expressions
int base = 2;
int offset = 3;
int computed = base + offset;

print("arr[" + computed + "] = " + arr[computed]);

// Test with multiplication
int multiplier = 2;
int index1 = base * multiplier;
print("arr[" + index1 + "] = " + arr[index1]);

// Test with division
int divisor = 2;
int index2 = arr.length / divisor;
print("arr[" + index2 + "] = " + arr[index2]);

// Test with modulo
int mod = arr.length % 3;
print("arr[" + mod + "] = " + arr[mod]);

// Test expression in bounds
int safeIndex = (base + offset) % arr.length;
print("Safe computed index [" + safeIndex + "] = " + arr[safeIndex]);

print("Computed indices test completed");
