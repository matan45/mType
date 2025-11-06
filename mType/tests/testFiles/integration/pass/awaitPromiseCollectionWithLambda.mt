// Test: Promise arrays transformed with lambdas
// @Script

class PromiseCollectionTest {
    public function async processAll() : Promise<Int> {
        Promise<Int>[] promises = [];

        (Int) -> Promise<Int> createPromise = async (value: Int) : Promise<Int> => {
            await delay(5);
            return value * value;
        };

        promises.push(createPromise(2));
        promises.push(createPromise(3));
        promises.push(createPromise(4));

        Int[] results = [];
        Int i = 0;
        while (i < promises.length()) {
            Int result = await promises[i];
            results.push(result);
            i = i + 1;
        }

        // Sum all results
        Int sum = 0;
        Int j = 0;
        while (j < results.length()) {
            sum = sum + results[j];
            j = j + 1;
        }

        print("Sum of squares: " + sum.toString()); // 4 + 9 + 16 = 29
        return sum;
    }

    public function async mapWithAsync() : Promise<String> {
        (Int) -> Promise<String> transform = async (x: Int) : Promise<String> => {
            await delay(5);
            return "Item-" + x.toString();
        };

        Int[] items = [1, 2, 3];
        String[] transformed = [];

        Int i = 0;
        while (i < items.length()) {
            String result = await transform(items[i]);
            transformed.push(result);
            i = i + 1;
        }

        // Join results
        String result = "";
        Int j = 0;
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

function async delay(ms: Int) : Promise<void> {
    // Simulated delay
}

function async main() : Promise<void> {
    PromiseCollectionTest test = new PromiseCollectionTest();
    Int sum = await test.processAll();
    assert(sum == 29, "Promise collection should compute correctly");

    String mapped = await test.mapWithAsync();
    print("Mapped: " + mapped);
    assert(mapped == "Item-1, Item-2, Item-3", "Async mapping should work correctly");
}
