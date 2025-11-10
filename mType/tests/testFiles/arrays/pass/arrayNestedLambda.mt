// Test array modifications in nested lambdas
print("Testing nested lambda with arrays");

interface NestedOperation {
    function outer() : int;
}

int[] numbers = new int[4];
numbers[0] = 1;
numbers[1] = 2;
numbers[2] = 3;
numbers[3] = 4;

print("Initial array: [1, 2, 3, 4]");

// Nested lambda structure
interface InnerOp {
    function inner(int x) : int;
}

InnerOp multiplier = x -> {
    return x * 2;
};

// Apply inner operation to array
for (int i = 0; i < numbers.length; i++) {
    numbers[i] = multiplier.inner(numbers[i]);
}

print("After doubling:");
for (int i = 0; i < numbers.length; i++) {
    print("  " + numbers[i]);
}

// Another nested operation
InnerOp adder = x -> {
    return x + 10;
};

for (int i = 0; i < numbers.length; i++) {
    numbers[i] = adder.inner(numbers[i]);
}

print("After adding 10:");
for (int i = 0; i < numbers.length; i++) {
    print("  " + numbers[i]);
}

print("Nested lambda test completed");
