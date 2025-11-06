// Test: Pipeline of async lambdas
// @Script

class AsyncPipelineTest {
    public function async executePipeline() : Promise<Int> {
        var step1 = async (Int x) : Promise<Int> => {
            await delay(5);
            print("Step 1: " + x.toString() + " -> " + (x + 5).toString());
            return x + 5;
        };

        var step2 = async (Int x) : Promise<Int> => {
            await delay(5);
            print("Step 2: " + x.toString() + " -> " + (x * 2).toString());
            return x * 2;
        };

        var step3 = async (Int x) : Promise<Int> => {
            await delay(5);
            print("Step 3: " + x.toString() + " -> " + (x - 3).toString());
            return x - 3;
        };

        // Pipeline: 10 -> +5 -> *2 -> -3 = 27
        Int input = 10;
        Int result1 = await step1(input);
        Int result2 = await step2(result1);
        Int result3 = await step3(result2);

        print("Final result: " + result3.toString());
        return result3;
    }

    public function async conditionalPipeline(Int value) : Promise<String> {
        var check = async (Int x) : Promise<Bool> => {
            await delay(5);
            return x > 10;
        };

        var processHigh = async (Int x) : Promise<String> => {
            await delay(5);
            return "High: " + x.toString();
        };

        var processLow = async (Int x) : Promise<String> => {
            await delay(5);
            return "Low: " + x.toString();
        };

        Bool isHigh = await check(value);
        String result = "";
        if (isHigh) {
            result = await processHigh(value);
        } else {
            result = await processLow(value);
        }

        return result;
    }
}

function async delay(Int ms) : Promise<void> {
    // Simulated delay
}

function async main() : Promise<void> {
    AsyncPipelineTest test = new AsyncPipelineTest();
    Int result1 = await test.executePipeline();
    assert(result1 == 27, "Pipeline should compute correctly");

    String result2 = await test.conditionalPipeline(15);
    assert(result2 == "High: 15", "Conditional pipeline should work");

    String result3 = await test.conditionalPipeline(5);
    assert(result3 == "Low: 5", "Conditional pipeline should handle low values");
}
