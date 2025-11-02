// Test: Async lambda capturing this
// @Script

class AsyncThisTest {
    private multiplier: Int;
    private prefix: String;

    constructor(m: Int, p: String) {
        this.multiplier = m;
        this.prefix = p;
    }

    async processWithThis() : Promise<String> {
        let operation = async (x: Int) : Promise<String> => {
            await delay(5);
            let result = x * this.multiplier;
            return this.prefix + result.toString();
        };

        let result = await operation(7);
        print(result);
        return result;
    }

    async chainWithThis() : Promise<Int> {
        let step1 = async (x: Int) : Promise<Int> => {
            await delay(5);
            return x + this.multiplier;
        };

        let step2 = async (x: Int) : Promise<Int> => {
            await delay(5);
            return x * this.multiplier;
        };

        let temp = await step1(5);
        let final = await step2(temp);
        print("Chained with this: " + final.toString());
        return final;
    }

    getMultiplier() : Int {
        return this.multiplier;
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async main() : Promise<Void> {
    let test = new AsyncThisTest(3, "Result: ");
    let result1 = await test.processWithThis();
    assert(result1 == "Result: 21", "Async lambda should capture this correctly");

    let result2 = await test.chainWithThis();
    assert(result2 == 24, "Chained async lambdas should use this correctly"); // (5 + 3) * 3 = 24
}
