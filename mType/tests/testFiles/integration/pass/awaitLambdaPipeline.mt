// Test: Pipeline of async lambdas
// @Script

class AsyncPipelineTest {
    async executePipeline() : Promise<Int> {
        let step1 = async (x: Int) : Promise<Int> => {
            await delay(5);
            print("Step 1: " + x.toString() + " -> " + (x + 5).toString());
            return x + 5;
        };

        let step2 = async (x: Int) : Promise<Int> => {
            await delay(5);
            print("Step 2: " + x.toString() + " -> " + (x * 2).toString());
            return x * 2;
        };

        let step3 = async (x: Int) : Promise<Int> => {
            await delay(5);
            print("Step 3: " + x.toString() + " -> " + (x - 3).toString());
            return x - 3;
        };

        // Pipeline: 10 -> +5 -> *2 -> -3 = 27
        let input: Int = 10;
        let result1 = await step1(input);
        let result2 = await step2(result1);
        let result3 = await step3(result2);

        print("Final result: " + result3.toString());
        return result3;
    }

    async conditionalPipeline(value: Int) : Promise<String> {
        let check = async (x: Int) : Promise<Bool> => {
            await delay(5);
            return x > 10;
        };

        let processHigh = async (x: Int) : Promise<String> => {
            await delay(5);
            return "High: " + x.toString();
        };

        let processLow = async (x: Int) : Promise<String> => {
            await delay(5);
            return "Low: " + x.toString();
        };

        let isHigh = await check(value);
        let result: String = "";
        if (isHigh) {
            result = await processHigh(value);
        } else {
            result = await processLow(value);
        }

        return result;
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async main() : Promise<Void> {
    let test = new AsyncPipelineTest();
    let result1 = await test.executePipeline();
    assert(result1 == 27, "Pipeline should compute correctly");

    let result2 = await test.conditionalPipeline(15);
    assert(result2 == "High: 15", "Conditional pipeline should work");

    let result3 = await test.conditionalPipeline(5);
    assert(result3 == "Low: 5", "Conditional pipeline should handle low values");
}
