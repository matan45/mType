// For-each type mismatch error test

// This should fail: wrong variable type in for-each loop
Array<int> numbers = [1, 2, 3, 4, 5];

// Type mismatch: trying to use string variable with int array
for (string num : numbers) {
    print(num);
}

print("This should not print");