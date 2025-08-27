class Result {
    bool success;
    string message;
    int value;
    
    constructor(bool s, string m, int v) {
        success = s;
        message = m;
        value = v;
    }
}

function performOperation(int input): Result {
    if (input < 0) {
        return new Result(false, "Negative input", 0);
    }
    if (input > 100) {
        return new Result(false, "Input too large", 0);
    }
    
    int result = input * 2;
    return new Result(true, "Success", result);
}

Result r1 = performOperation(50);
print(r1.success); // true
print(r1.message); // Success
print(r1.value); // 100

Result r2 = performOperation(-5);
print(r2.success); // false
print(r2.message); // Negative input
print(r2.value); // 0

Result r3 = performOperation(150);
print(r3.success); // false
print(r3.message); // Input too large
print(r3.value); // 0