// Test literals with function calls (side effects)
print("Testing literals with side effects");

int counter = 0;

int increment() {
    counter = counter + 1;
    return counter;
}

print("Counter before: " + counter);

int[] withSideEffects = [
    increment(),
    increment(),
    increment(),
    increment(),
    increment()
];

print("Counter after: " + counter);
print("Array elements:");
for (int i = 0; i < withSideEffects.length; i++) {
    print("  withSideEffects[" + i + "] = " + withSideEffects[i]);
}

// Nested arrays with side effects
int resetCounter() {
    int old = counter;
    counter = 0;
    return old;
}

counter = 100;
int[][] nested = [
    [increment(), increment()],
    [increment(), resetCounter()],
    [increment(), increment()]
];

print("After nested initialization:");
print("Counter value: " + counter);
print("Nested array:");
for (int i = 0; i < nested.length; i++) {
    for (int j = 0; j < nested[i].length; j++) {
        print("  nested[" + i + "][" + j + "] = " + nested[i][j]);
    }
}

print("Side effects test completed");
