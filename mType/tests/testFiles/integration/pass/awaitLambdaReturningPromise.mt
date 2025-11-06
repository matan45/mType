// Test: Lambda returning Promise<T>
// @Script

class PromiseLambdaTest {
    public function async processData() : Promise<String> {
        var fetchData = async (Int id) : Promise<String> => {
            await delay(5);
            return "Data-" + id.toString();
        };

        String result = await fetchData(42);
        print("Fetched: " + result);
        return result;
    }

    public function async chainPromises() : Promise<Int> {
        var step1 = async (Int x) : Promise<Int> => {
            await delay(5);
            return x * 2;
        };

        var step2 = async (Int x) : Promise<Int> => {
            await delay(5);
            return x + 10;
        };

        Int intermediate = await step1(5);
        Int final = await step2(intermediate);
        return final;
    }
}

function async delay(Int ms) : Promise<void> {
    // Simulated delay
}

function async main() : Promise<void> {
    PromiseLambdaTest test = new PromiseLambdaTest();
    String result1 = await test.processData();
    assert(result1 == "Data-42", "Lambda should return correct promise value");

    Int result2 = await test.chainPromises();
    assert(result2 == 20, "Chained promise lambdas should compute correctly");
}
