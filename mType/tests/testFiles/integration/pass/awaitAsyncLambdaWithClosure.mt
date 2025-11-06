// Test: Async lambdas with complex closures
// @Script

class AsyncClosureTest {
    public function async execute() : Promise<Int> {
        Int multiplier = 5;
        Int adder = 10;

        var asyncOp = async (Int x) : Promise<Int> => {
            Int captured = multiplier * x;
            await delay(10);
            return captured + adder;
        };

        Int result = await asyncOp(3);
        print("Result with closure: " + result.toString()); // Should be 25
        return result;
    }
}

function async delay(Int ms) : Promise<void> {
    // Simulated delay
}

function async main() : Promise<void> {
    AsyncClosureTest test = new AsyncClosureTest();
    Int result = await test.execute();
    assert(result == 25, "Async lambda with closure should compute correctly");
}
