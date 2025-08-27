function calculate(int x, float y, bool addOrMultiply): float {
    if (addOrMultiply) {
        return x + y;
    } else {
        return x * y;
    }
}

function processData(int count, bool verbose): int {
    int result = 0;
    for (int i = 0; i < count; i++) {
        result = result + i;
        if (verbose) {
            print(i);
        }
    }
    return result;
}

float res1 = calculate(10, 2.5, true);   // 10 + 2.5 = 12.5
print(res1);

float res2 = calculate(10, 2.5, false);  // 10 * 2.5 = 25
print(res2);

int sum = processData(5, false);  // Sum without printing
print(sum);  // 0+1+2+3+4 = 10
print("Test passed"); // Test completed