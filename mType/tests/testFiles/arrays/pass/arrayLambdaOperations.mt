// Test forEach, map, filter with lambdas
print("Testing array operations with lambdas");

interface ArrayProcessor {
    function process(int[] arr) : void;
}

interface ArrayTransform {
    function transform(int x) : int;
}

int[] numbers = new int[5];
for (int i = 0; i < numbers.length; i++) {
    numbers[i] = i + 1;
}

print("Original array: [1, 2, 3, 4, 5]");

// Manual forEach with lambda
ArrayProcessor printer = arr -> {
    for (int i = 0; i < arr.length; i++) {
        print("  Element: " + arr[i]);
    }
};

print("ForEach operation:");
printer.process(numbers);

// Manual map operation
ArrayTransform doubler = x -> {
    return x * 2;
};

int[] doubled = new int[numbers.length];
for (int i = 0; i < numbers.length; i++) {
    doubled[i] = doubler.transform(numbers[i]);
}

print("Mapped (doubled):");
for (int i = 0; i < doubled.length; i++) {
    print("  " + doubled[i]);
}

print("Lambda operations test completed");
