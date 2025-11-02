// Test: Exception handling in async lambdas
// @Script

class AsyncExceptionTest {
    async testCatchInLambda() : Promise<String> {
        let safeOp = async (x: Int) : Promise<String> => {
            try {
                await delay(5);
                if (x < 0) {
                    throw "Negative value not allowed";
                }
                return "Success: " + x.toString();
            } catch (e: String) {
                return "Error: " + e;
            }
        };

        let result1 = await safeOp(10);
        print(result1);

        let result2 = await safeOp(-5);
        print(result2);

        return result2;
    }

    async testRethrow() : Promise<Int> {
        let riskyOp = async (x: Int) : Promise<Int> => {
            await delay(5);
            if (x == 0) {
                throw "Division by zero";
            }
            return 100 / x;
        };

        try {
            let result = await riskyOp(10);
            print("Result: " + result.toString());
            return result;
        } catch (e: String) {
            print("Caught: " + e);
            return -1;
        }
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async main() : Promise<Void> {
    let test = new AsyncExceptionTest();
    let result1 = await test.testCatchInLambda();
    assert(result1 == "Error: Negative value not allowed", "Should handle exception in lambda");

    let result2 = await test.testRethrow();
    assert(result2 == 10, "Should compute division correctly");
}
