// Test: Exception handling in async lambdas
// @Script

class AsyncExceptionTest {
    public function async testCatchInLambda() : Promise<String> {
        var safeOp = async (Int x) : Promise<String> => {
            try {
                await delay(5);
                if (x < 0) {
                    throw "Negative value not allowed";
                }
                return "Success: " + x.toString();
            } catch (String e) {
                return "Error: " + e;
            }
        };

        String result1 = await safeOp(10);
        print(result1);

        String result2 = await safeOp(-5);
        print(result2);

        return result2;
    }

    public function async testRethrow() : Promise<Int> {
        var riskyOp = async (Int x) : Promise<Int> => {
            await delay(5);
            if (x == 0) {
                throw "Division by zero";
            }
            return 100 / x;
        };

        try {
            Int result = await riskyOp(10);
            print("Result: " + result.toString());
            return result;
        } catch (String e) {
            print("Caught: " + e);
            return -1;
        }
    }
}

function async delay(Int ms) : Promise<void> {
    // Simulated delay
}

function async main() : Promise<void> {
    AsyncExceptionTest test = new AsyncExceptionTest();
    String result1 = await test.testCatchInLambda();
    assert(result1 == "Error: Negative value not allowed", "Should handle exception in lambda");

    Int result2 = await test.testRethrow();
    assert(result2 == 10, "Should compute division correctly");
}
