// Test: Importing async generic interfaces with lambdas
// @Script

import * from "modules/AsyncGenericInterface.mt";

class AsyncCollectionProcessor<T, R> implements AsyncMapper<T, R>, AsyncFilter<T> {
    function async map(items: T[], transformer: (T) -> R) : Promise<R[]> {
        await delay(5);
        R[] result = [];
        Int i = 0;
        while (i < items.length()) {
            result.push(transformer(items[i]));
            i = i + 1;
        }
        return result;
    }

    function async mapAsync(items: T[], transformer: (T) -> Promise<R>) : Promise<R[]> {
        await delay(5);
        R[] result = [];
        Int i = 0;
        while (i < items.length()) {
            let mapped = await transformer(items[i]);
            result.push(mapped);
            i = i + 1;
        }
        return result;
    }

    function async filter(items: T[], predicate: (T) -> Bool) : Promise<T[]> {
        await delay(5);
        T[] result = [];
        Int i = 0;
        while (i < items.length()) {
            if (predicate(items[i])) {
                result.push(items[i]);
            }
            i = i + 1;
        }
        return result;
    }

    function async filterAsync(items: T[], predicate: (T) -> Promise<Bool>) : Promise<T[]> {
        await delay(5);
        T[] result = [];
        Int i = 0;
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

function async delay(ms: Int) : Promise<void> {
    // Simulated delay
}

function async main() : Promise<void> {
    let processor = new AsyncCollectionProcessor<Int, String>();

    Int[] numbers = [1, 2, 3, 4, 5];

    // Test sync map with lambda
    let strings = await processor.map(numbers, (n: Int) : String => {
        return "Item-" + n.toString();
    });
    print("Mapped: " + strings[0]);
    assert(strings[0] == "Item-1", "Should map synchronously");

    // Test async map with lambda
    let asyncStrings = await processor.mapAsync(numbers, function async (n: Int) : Promise<String> => {
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
    let asyncEvens = await filterProc.filterAsync(numbers, function async (n: Int) : Promise<Bool> => {
        await delay(5);
        return n % 2 == 0;
    });
    assert(asyncEvens.length() == 2, "Should filter asynchronously");

    print("All features integration test passed");
}
