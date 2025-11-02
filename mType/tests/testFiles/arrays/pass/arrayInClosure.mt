// Test arrays captured in closures
print("Testing arrays in closures");

int[] createCounter() {
    int[] counts = new int[3];
    counts[0] = 0;
    counts[1] = 0;
    counts[2] = 0;
    return counts;
}

int[] counter = createCounter();

void increment(int[] arr, int index) {
    arr[index] = arr[index] + 1;
}

print("Initial counts:");
for (int i = 0; i < counter.length; i++) {
    print("  counter[" + i + "] = " + counter[i]);
}

increment(counter, 0);
increment(counter, 0);
increment(counter, 1);
increment(counter, 2);
increment(counter, 2);
increment(counter, 2);

print("After increments:");
for (int i = 0; i < counter.length; i++) {
    print("  counter[" + i + "] = " + counter[i]);
}

print("Arrays in closures test completed");
