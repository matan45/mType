// Test lambda capturing arrays
print("Testing lambda capturing arrays");

interface ArrayOperation {
    function execute() : int;
}

int[] data = new int[5];
for (int i = 0; i < data.length; i++) {
    data[i] = (i + 1) * 10;
}

print("Array: [10, 20, 30, 40, 50]");

// Lambda that captures array and calculates sum
ArrayOperation sumCalculator = () -> {
    int total = 0;
    for (int i = 0; i < data.length; i++) {
        total = total + data[i];
    }
    return total;
};

int sum = sumCalculator.execute();
print("Sum calculated by lambda: " + sum);

// Modify array
data[0] = 100;
data[4] = 500;

// Lambda sees modified array
int newSum = sumCalculator.execute();
print("Sum after modification: " + newSum);

print("Lambda capture test completed");
