// Test file for O1/O2 optimization - Dead Code Elimination in Async Functions
// Tests unreachable code after return/throw statements in async functions, methods, and lambdas

import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Test 1: Async function with dead code after return
function async fetchData(int id): Promise<String> {
    print("Fetching data for id: " + id);
    if (id < 0) {
        return new String("error");
        print("Dead after return in async function");  // Should be removed
        id = 0;                                         // Should be removed
    }

    return new String("data_" + id);
}

// Test 2: Async function with throw and dead code
function async validateInput(string input): Promise<void> {
    print("Validating input: " + input);

    if (input == "") {
        throw new Exception("Empty input not allowed");
        print("Dead after throw in async");  // Should be removed
        input = "default";                    // Should be removed
    }

    print("Input is valid");
	return null;
}

function async innerProcess(int x): Promise<Int> {
        if (x < 0) {
            return new Int(0);
            print("Dead in inner async");  // Should be removed
        }
        return new Int(x + 10);
    }

// Test 3: Nested async function with dead code
function async  processData(int value): Promise<Int> {
    print("Processing: " + value);

    if (value > 100) {
        return new Int(value * 2);
        print("Dead in outer async");  // Should be removed
    }

    Int result = await innerProcess(value);
    return result;
}

// Test 4: Async method in class with dead code
class DataService {
    private string apiUrl;

    constructor(string url) {
        this.apiUrl = url;
    }

    public function async fetchUser(int userId): Promise<String> {
        print("Fetching user: " + userId);

        if (userId <= 0) {
            return new String("invalid_user");
            print("Dead after return in async method");  // Should be removed
            userId = 1;                                   // Should be removed
        }

        return new String("user_" + userId);
    }

    public function async updateUser(int userId, string data): Promise<void> {
        print("Updating user: " + userId);

        if (data == "") {
            throw new Exception("Empty data");
            print("Dead after throw in async method");  // Should be removed
        }

        print("User updated successfully");
		return null;
    }

    // UNUSED async method - should be removed by unused code elimination
    private function async unusedAsyncMethod(): Promise<void> {
        print("This is never called");
        return null;
        print("Dead code in unused async method");
    }
}

// Test 5: Async lambda with dead code
interface AsyncProcessor<T> {
    function async process(T value): T;
}

function async testAsyncLambda(): Promise<void> {
    AsyncProcessor<Int> processor = async x -> {
        print("Processing in async lambda: " + x);

        if (x.value == 0) {
            return 1;
            print("Dead after return in async lambda");  // Should be removed
            x.value = 10;                                       // Should be removed
        }

        return new Int(x.value * 2);
    };

    Int result = await processor.process(new Int(5));
    print("Async lambda result: " + result.toString());
	return null;
}

// Test 6: Async function with multiple returns
function async classifyValue(int x): Promise<String> {
    if (x < 0) {
        return new String("negative");
    }
    if (x == 0) {
        return new String("zero");
    }
    if (x > 0) {
        return new String("positive");
    }
    print("Dead after all returns in async");  // Should be removed
    return new String("unknown");                           // Should be removed
}

// Test 7: Async function with try-catch and dead code
function async  riskyOperation(int code): Promise<String> {
    try {
        if (code == 1) {
            throw new Exception("Error 1");
        }
        if (code == 2) {
            return new String("success");
            print("Dead after return in async try");  // Should be removed
        }
        print("Try block continues");
    } catch (Exception e) {
        print("Caught in async: " + e.getMessage());
        return new String("error_handled");
        print("Dead after return in async catch");  // Should be removed
    }

    print("This is reachable if code is not 1 or 2");
    return new String("completed");
}

// Test 8: Async function with while loop and dead code
function async processItems(int count): Promise<Int> {
    int sum = 0;
    int i = 0;

    while (i < count) {
        sum = sum + i;

        if (sum > 50) {
            return new Int(sum);
            print("Dead after return in async while");  // Should be removed
            break;                                       // Should be removed
        }

        i = i + 1;
    }

    return new Int(sum);
}

// Test 9: Async method with nested if-else and dead code
class AsyncCalculator {
    public function async compute(int a, int b, string operation): Promise<Int> {
        if (operation == "add") {
            return new Int(a + b);
            print("Dead after add");  // Should be removed
        } else if (operation == "subtract") {
            return new Int(a - b);
            print("Dead after subtract");  // Should be removed
        } else {
            return new Int(0);
            print("Dead after default");  // Should be removed
        }

        print("Dead after complete if-else chain");  // Should be removed
        return new Int(-1);                                    // Should be removed
    }
}

// Entry point
print("=== Testing Async Function Dead Code Elimination ===");

String result1 = await fetchData(10);
print("fetchData result: " + result1.toString());

String result2 = await fetchData(-1);
print("fetchData error: " + result2.toString());

try {
    await validateInput("");
} catch (Exception e) {
    print("Caught validation exception");
}

Int result3 = await processData(150);
print("processData result: " + result3.toString());

DataService service = new DataService("https://api.example.com");
String user = await service.fetchUser(42);
print("User: " + user.toString());

try {
    await service.updateUser(1, "");
} catch (Exception e) {
    print("Caught update exception");
}

testAsyncLambda();

String classification = await classifyValue(-10);
print("Classification: " + classification.toString());

String riskyResult = await riskyOperation(2);
print("Risky operation: " + riskyResult.toString());

Int sumResult = await processItems(20);
print("Process items sum: " + sumResult.toString());

AsyncCalculator calc = new AsyncCalculator();
Int computeResult = await calc.compute(10, 5, "add");
print("Compute result: " + computeResult.toString());

print("Async Dead Code Test Complete!");
