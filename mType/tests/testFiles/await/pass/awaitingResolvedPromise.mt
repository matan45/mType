// Test awaiting already-resolved promises
// Edge case: Promise that is already resolved when awaited
import "../../lib/primitives/Int.mt";
class Value {
    int data;

    public constructor(int d) {
        this.data = d;
    }

    public function getData(): int {
        return this.data;
    }
}

print("=== Awaiting Already-Resolved Promise Test ===");

// Test 1: Immediate return creates resolved promise
function async getImmediateValue(): Promise<Int> {
    // This immediately returns a resolved promise
    return new Int(42);
}

function async testImmediateResolve(): Promise<Int> {
    // The promise is already resolved by the time we await it
    Int value = await getImmediateValue();
    print("Immediate resolve value: " + value.toString());
    return value;
}

// Test 2: Store promise then await (Promise storage)
function async testPromiseStorage(): Promise<Int> {
    // Store the promise without awaiting immediately
    Promise<Int> promise = getImmediateValue();
    print("Promise stored successfully");

    // Await it later
    Int value = await promise;
    print("Promise storage result: " + value.toString());
    return value;
}

// Test 3: Multiple awaits on same resolved value
function async getSingleValue(): Promise<Value> {
    Value v = new Value(100);
    return v;
}

function async testMultipleAwaitsOnResolved(): Promise<Int> {
    // First await
    Value v1 = await getSingleValue();
    print("First await: " + v1.getData());

    // Second await on same function (different promise, but immediately resolved)
    Value v2 = await getSingleValue();
    print("Second await: " + v2.getData());

    // Both should work even though promises are resolved immediately
    int total = v1.getData() + v2.getData();
    return new Int(total);
}

// Test 4: Chain of immediately resolving promises
function async chainLink1(): Promise<Int> {
    return new Int(10);
}

function async chainLink2(): Promise<Int> {
    Int val = await chainLink1();
    return new Int(val.value + 5);
}

function async chainLink3(): Promise<Int> {
    Int val = await chainLink2();
    return new Int(val.value + 3);
}

function async testResolvedChain(): Promise<Int> {
    // Each link in the chain resolves immediately
    Int result = await chainLink3();
    print("Resolved chain result: " + result);
    return result;
}

// Test 5: Conditional early returns
function async conditionalReturn(bool condition): Promise<Int> {
    if (condition) {
        // Early return - promise resolves immediately
        return new Int(999);
    }

    // This also resolves immediately
    return new Int(111);
}

function async testConditionalResolve(): Promise<Int> {
    Int val1 = await conditionalReturn(true);
    print("Conditional true: " + val1);

    Int val2 = await conditionalReturn(false);
    print("Conditional false: " + val2);

    int total = val1.value + val2.value;
    return new Int(total);
}

// Main function to run all tests
function async main(): Promise<Int> {
    Int r1 = await testImmediateResolve();
    Int r2 = await testPromiseStorage();
    Int r3 = await testMultipleAwaitsOnResolved();
    Int r4 = await testResolvedChain();
    Int r5 = await testConditionalResolve();

    print("All resolved promise tests completed");
    int total = r1.value + r2.value + r3.value + r4.value + r5.value;
    print("Total: " + total);
    return new Int(total);
}

main();
