// Capturing loop variables correctly test
interface Function {
    function apply(int x) : int;
}

print("=== Capture In Loop Test ===");

// Create multiple lambdas in a loop
Function[] funcs = new Function[5];

for (int i = 0; i < 5; i = i + 1) {
    int captured = i;  // Capture loop variable
    funcs[i] = x -> captured + x;
}

// Test each lambda
for (int i = 0; i < 5; i = i + 1) {
    print("funcs[" + i + "](10) = " + funcs[i].apply(10));
}

// More complex loop capture
Function[] multipliers = new Function[3];
int[] bases = [10, 20, 30];

for (int i = 0; i < 3; i = i + 1) {
    int base = bases[i];
    int index = i;
    multipliers[i] = x -> {
        return base * x + index;
    };
}

print("Multipliers:");
for (int i = 0; i < 3; i = i + 1) {
    print("mult[" + i + "](5) = " + multipliers[i].apply(5));
}

// Nested loop capture
print("Nested loops:");
int counter = 0;
for (int i = 0; i < 2; i = i + 1) {
    for (int j = 0; j < 2; j = j + 1) {
        int ci = i;
        int cj = j;
        Function f = x -> ci * 10 + cj * 100 + x;
        print("i=" + i + " j=" + j + ": " + f.apply(1));
        counter = counter + 1;
    }
}

print("Capture in loop complete");
