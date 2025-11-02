// Test: Promise arrays transformed with lambdas
// @Script

class PromiseCollectionTest {
    async processAll() : Promise<Int> {
        let promises: Promise<Int>[] = [];

        let createPromise = async (value: Int) : Promise<Int> => {
            await delay(5);
            return value * value;
        };

        promises.push(createPromise(2));
        promises.push(createPromise(3));
        promises.push(createPromise(4));

        let results: Int[] = [];
        let i: Int = 0;
        while (i < promises.length()) {
            let result = await promises[i];
            results.push(result);
            i = i + 1;
        }

        // Sum all results
        let sum: Int = 0;
        let j: Int = 0;
        while (j < results.length()) {
            sum = sum + results[j];
            j = j + 1;
        }

        print("Sum of squares: " + sum.toString()); // 4 + 9 + 16 = 29
        return sum;
    }

    async mapWithAsync() : Promise<String> {
        let transform = async (x: Int) : Promise<String> => {
            await delay(5);
            return "Item-" + x.toString();
        };

        let items: Int[] = [1, 2, 3];
        let transformed: String[] = [];

        let i: Int = 0;
        while (i < items.length()) {
            let result = await transform(items[i]);
            transformed.push(result);
            i = i + 1;
        }

        // Join results
        let result: String = "";
        let j: Int = 0;
        while (j < transformed.length()) {
            if (j > 0) {
                result = result + ", ";
            }
            result = result + transformed[j];
            j = j + 1;
        }

        return result;
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async main() : Promise<Void> {
    let test = new PromiseCollectionTest();
    let sum = await test.processAll();
    assert(sum == 29, "Promise collection should compute correctly");

    let mapped = await test.mapWithAsync();
    print("Mapped: " + mapped);
    assert(mapped == "Item-1, Item-2, Item-3", "Async mapping should work correctly");
}
