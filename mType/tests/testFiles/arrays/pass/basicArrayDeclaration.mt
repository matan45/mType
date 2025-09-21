// Test basic array declaration and initialization
print("Testing basic array declaration");

// Basic array creation
int[] numbers = new int[3];
print("Integer array created with length: " + numbers.length);

string[] names = new string[2];
print("String array created with length: " + names.length);

// Array element assignment (using .set for now)
numbers.set(0, 10);
numbers.set(1, 20);
numbers.set(2, 30);

// Array element access using bracket syntax
print("numbers[0] = " + numbers[0]);
print("numbers[1] = " + numbers[1]);
print("numbers[2] = " + numbers[2]);

// String array operations
names.set(0, "Alice");
names.set(1, "Bob");

print("names[0] = " + names[0]);
print("names[1] = " + names[1]);

// Test arrays in loops
print("Testing arrays in loops:");
for (int i = 0; i < numbers.length; i++) {
    print("  numbers[" + i + "] = " + numbers[i]);
}

print("Basic array declaration test completed");