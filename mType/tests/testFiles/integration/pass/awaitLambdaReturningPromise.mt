// Test: Lambda returning Promise<T>
// @Script

class PromiseLambdaTest {
    async processData() : Promise<String> {
        let fetchData = async (id: Int) : Promise<String> => {
            await delay(5);
            return "Data-" + id.toString();
        };

        let result = await fetchData(42);
        print("Fetched: " + result);
        return result;
    }

    async chainPromises() : Promise<Int> {
        let step1 = async (x: Int) : Promise<Int> => {
            await delay(5);
            return x * 2;
        };

        let step2 = async (x: Int) : Promise<Int> => {
            await delay(5);
            return x + 10;
        };

        let intermediate = await step1(5);
        let final = await step2(intermediate);
        return final;
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async main() : Promise<Void> {
    let test = new PromiseLambdaTest();
    let result1 = await test.processData();
    assert(result1 == "Data-42", "Lambda should return correct promise value");

    let result2 = await test.chainPromises();
    assert(result2 == 20, "Chained promise lambdas should compute correctly");
}
