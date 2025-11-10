// Test off-by-one error in loops
print("Testing off-by-one error");

int[] numbers = new int[5];
for (int i = 0; i < numbers.length; i++) {
    numbers[i] = i * 10;
}

// Off-by-one error: using <= instead of <
for (int i = 0; i <= numbers.length; i++) {
    print("numbers[" + i + "] = " + numbers[i]);
}

print("This should not be reached");
