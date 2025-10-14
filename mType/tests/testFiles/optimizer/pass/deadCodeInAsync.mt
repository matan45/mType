// Test file for O1/O2 optimization - Dead Code Elimination in Async Functions
// Tests unreachable code after return/throw statements in async functions, methods, and lambdas

import * from "../../lib/exceptions/Exception.mt";

// Test 1: Async function with dead code after return
async function fetchData(int id): string {
    print("Fetching data for id: " + id);
    if (id < 0) {
        return "error";
        print("Dead after return in async function");  // Should be removed
        id = 0;                                         // Should be removed
    }

    return "data_" + id;
}

// Test 2: Async function with throw and dead code
async function validateInput(string input): void {
    print("Validating input: " + input);

    if (input == "") {
        throw new Exception("Empty input not allowed");
        print("Dead after throw in async");  // Should be removed
        input = "default";                    // Should be removed
    }

    print("Input is valid");
}

// Test 3: Nested async function with dead code
async function processData(int value): int {
    print("Processing: " + value);

    if (value > 100) {
        return value * 2;
        print("Dead in outer async");  // Should be removed
    }

    async function innerProcess(int x): int {
        if (x < 0) {
            return 0;
            print("Dead in inner async");  // Should be removed
        }
        return x + 10;
    }

    int result = await innerProcess(value);
    return result;
}

// Test 4: Async method in class with dead code
class DataService {
    private string apiUrl;

    constructor(string url) {
        this.apiUrl = url;
    }

    public async function fetchUser(int userId): string {
        print("Fetching user: " + userId);

        if (userId <= 0) {
            return "invalid_user";
            print("Dead after return in async method");  // Should be removed
            userId = 1;                                   // Should be removed
        }

        return "user_" + userId;
    }

    public async function updateUser(int userId, string data): void {
        print("Updating user: " + userId);

        if (data == "") {
            throw new Exception("Empty data");
            print("Dead after throw in async method");  // Should be removed
        }

        print("User updated successfully");
    }

    // UNUSED async method - should be removed by unused code elimination
    private async function unusedAsyncMethod(): void {
        print("This is never called");
        return;
        print("Dead code in unused async method");
    }
}

// Test 5: Async lambda with dead code
interface AsyncProcessor<T> {
    async function process(T value): T;
}

function testAsyncLambda(): void {
    AsyncProcessor<int> processor = async (int x) -> {
        print("Processing in async lambda: " + x);

        if (x == 0) {
            return 1;
            print("Dead after return in async lambda");  // Should be removed
            x = 10;                                       // Should be removed
        }

        return x * 2;
    };

    int result = await processor.process(5);
    print("Async lambda result: " + result);
}

// Test 6: Async function with multiple returns
async function classifyValue(int x): string {
    if (x < 0) {
        return "negative";
    }
    if (x == 0) {
        return "zero";
    }
    if (x > 0) {
        return "positive";
    }
    print("Dead after all returns in async");  // Should be removed
    return "unknown";                           // Should be removed
}

// Test 7: Async function with try-catch and dead code
async function riskyOperation(int code): string {
    try {
        if (code == 1) {
            throw new Exception("Error 1");
        }
        if (code == 2) {
            return "success";
            print("Dead after return in async try");  // Should be removed
        }
        print("Try block continues");
    } catch (Exception e) {
        print("Caught in async: " + e.getMessage());
        return "error_handled";
        print("Dead after return in async catch");  // Should be removed
    }

    print("This is reachable if code is not 1 or 2");
    return "completed";
}

// Test 8: Async function with while loop and dead code
async function processItems(int count): int {
    int sum = 0;
    int i = 0;

    while (i < count) {
        sum = sum + i;

        if (sum > 50) {
            return sum;
            print("Dead after return in async while");  // Should be removed
            break;                                       // Should be removed
        }

        i = i + 1;
    }

    return sum;
}

// Test 9: Async method with nested if-else and dead code
class AsyncCalculator {
    public async function compute(int a, int b, string operation): int {
        if (operation == "add") {
            return a + b;
            print("Dead after add");  // Should be removed
        } else if (operation == "subtract") {
            return a - b;
            print("Dead after subtract");  // Should be removed
        } else {
            return 0;
            print("Dead after default");  // Should be removed
        }

        print("Dead after complete if-else chain");  // Should be removed
        return -1;                                    // Should be removed
    }
}

// Entry point
print("=== Testing Async Function Dead Code Elimination ===");

string result1 = await fetchData(10);
print("fetchData result: " + result1);

string result2 = await fetchData(-1);
print("fetchData error: " + result2);

try {
    await validateInput("");
} catch (Exception e) {
    print("Caught validation exception");
}

int result3 = await processData(150);
print("processData result: " + result3);

DataService service = new DataService("https://api.example.com");
string user = await service.fetchUser(42);
print("User: " + user);

try {
    await service.updateUser(1, "");
} catch (Exception e) {
    print("Caught update exception");
}

testAsyncLambda();

string classification = await classifyValue(-10);
print("Classification: " + classification);

string riskyResult = await riskyOperation(2);
print("Risky operation: " + riskyResult);

int sumResult = await processItems(20);
print("Process items sum: " + sumResult);

AsyncCalculator calc = new AsyncCalculator();
int computeResult = await calc.compute(10, 5, "add");
print("Compute result: " + computeResult);

print("Async Dead Code Test Complete!");
