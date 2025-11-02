// Test: Importing async generic interfaces with lambdas
// @Script

import "modules/AsyncGenericInterface.mt";

class AsyncCollectionProcessor<T, R> : AsyncMapper<T, R>, AsyncFilter<T> {
    async map(items: T[], transformer: (T) -> R) : Promise<R[]> {
        await delay(5);
        let result: R[] = [];
        let i: Int = 0;
        while (i < items.length()) {
            result.push(transformer(items[i]));
            i = i + 1;
        }
        return result;
    }

    async mapAsync(items: T[], transformer: (T) -> Promise<R>) : Promise<R[]> {
        await delay(5);
        let result: R[] = [];
        let i: Int = 0;
        while (i < items.length()) {
            let mapped = await transformer(items[i]);
            result.push(mapped);
            i = i + 1;
        }
        return result;
    }

    async filter(items: T[], predicate: (T) -> Bool) : Promise<T[]> {
        await delay(5);
        let result: T[] = [];
        let i: Int = 0;
        while (i < items.length()) {
            if (predicate(items[i])) {
                result.push(items[i]);
            }
            i = i + 1;
        }
        return result;
    }

    async filterAsync(items: T[], predicate: (T) -> Promise<Bool>) : Promise<T[]> {
        await delay(5);
        let result: T[] = [];
        let i: Int = 0;
        while (i < items.length()) {
            let shouldInclude = await predicate(items[i]);
            if (shouldInclude) {
                result.push(items[i]);
            }
            i = i + 1;
        }
        return result;
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async main() : Promise<Void> {
    let processor = new AsyncCollectionProcessor<Int, String>();

    let numbers: Int[] = [1, 2, 3, 4, 5];

    // Test sync map with lambda
    let strings = await processor.map(numbers, (n: Int) : String => {
        return "Item-" + n.toString();
    });
    print("Mapped: " + strings[0]);
    assert(strings[0] == "Item-1", "Should map synchronously");

    // Test async map with lambda
    let asyncStrings = await processor.mapAsync(numbers, async (n: Int) : Promise<String> => {
        await delay(5);
        return "Async-" + n.toString();
    });
    print("Async mapped: " + asyncStrings[0]);
    assert(asyncStrings[0] == "Async-1", "Should map asynchronously");

    // Test filter
    let filterProc = new AsyncCollectionProcessor<Int, Int>();
    let evens = await filterProc.filter(numbers, (n: Int) : Bool => {
        return n % 2 == 0;
    });
    print("Evens: " + evens.length().toString());
    assert(evens.length() == 2, "Should filter correctly");

    // Test async filter
    let asyncEvens = await filterProc.filterAsync(numbers, async (n: Int) : Promise<Bool> => {
        await delay(5);
        return n % 2 == 0;
    });
    assert(asyncEvens.length() == 2, "Should filter asynchronously");

    print("All features integration test passed");
}
