// Lambda in tight loop (1000 iterations) test
interface Function {
    function apply(int x) : int;
}

interface VoidFunction {
    function execute(int x) : void;
}

print("=== Tight Loop Performance Test ===");

// Simple lambda in loop
Function doubler = x -> x * 2;
int sum1 = 0;
for (int i = 0; i < 1000; i = i + 1) {
    sum1 = sum1 + doubler.apply(i);
}
print("Sum of doubled 0-999: " + sum1);

// Lambda with more complex logic in loop
Function complex = x -> {
    int result = x;
    if (x % 2 == 0) {
        result = result * 2;
    } else {
        result = result + 10;
    }
    return result;
};

int sum2 = 0;
for (int i = 0; i < 1000; i = i + 1) {
    sum2 = sum2 + complex.apply(i);
}
print("Sum of complex 0-999: " + sum2);

// Multiple lambdas in loop
Function add10 = x -> x + 10;
Function mult3 = x -> x * 3;
Function sub5 = x -> x - 5;

int sum3 = 0;
for (int i = 0; i < 1000; i = i + 1) {
    int temp = add10.apply(i);
    temp = mult3.apply(temp);
    temp = sub5.apply(temp);
    sum3 = sum3 + temp;
}
print("Sum of pipeline 0-999: " + sum3);

// Lambda with captured variable in loop
int multiplier = 3;
Function capturedLambda = x -> x * multiplier;

int sum4 = 0;
for (int i = 0; i < 1000; i = i + 1) {
    sum4 = sum4 + capturedLambda.apply(i);
}
print("Sum with capture 0-999: " + sum4);

// Lambda modifying array in loop
int[] counters = [0, 0, 0, 0, 0];
VoidFunction counter = x -> {
    int index = x % 5;
    counters[index] = counters[index] + 1;
};

for (int i = 0; i < 1000; i = i + 1) {
    counter.execute(i);
}

print("Counter results:");
for (int i = 0; i < 5; i = i + 1) {
    print("counters[" + i + "] = " + counters[i]);
}

// Nested loop with lambda
Function nested = x -> x * x;
int sum5 = 0;
for (int i = 0; i < 100; i = i + 1) {
    for (int j = 0; j < 10; j = j + 1) {
        sum5 = sum5 + nested.apply(i + j);
    }
}
print("Nested loop sum: " + sum5);

// Lambda array processing in loop
int[] data = new int[100];
for (int i = 0; i < 100; i = i + 1) {
    data[i] = i + 1;
}

Function squarer = x -> x * x;
int sum6 = 0;
for (int i = 0; i < 100; i = i + 1) {
    data[i] = squarer.apply(data[i]);
    sum6 = sum6 + data[i];
}
print("Sum of squares 1-100: " + sum6);

print("Tight loop performance complete");
