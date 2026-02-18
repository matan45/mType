import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Bool.mt";
import * from "../../lib/collections/ArrayList.mt";

// Helper async functions
function async getValue(int n): Promise<Int> {
    return new Int(n);
}

function async getStringValue(string s): Promise<String> {
    return new String(s);
}

// Test 1: Promise reassignment
function async testPromiseReassignment(): Promise<Int> {
    print("--- Test 1: Promise Reassignment ---");
    Promise<Int> p = getValue(10);
    print("First promise stored");

    p = getValue(20);
    print("Promise reassigned");

    Int result = await p;
    print("Result after reassignment: " + result.toString());
    return result;
}

// Test 2: Null Promise
function async testNullPromise(): Promise<Int> {
    print("--- Test 2: Null Promise ---");
    Promise<Int>? p = null;
    print("Assigned null to Promise variable: success");

    // Note: awaiting null would cause an error, so we skip it
    // Int result = await p;
    return new Int(0);
}

// Test 3: Promise as class field
class PromiseContainer {
    Promise<Int> promiseField;

    public function async storePromise(): Promise<Int> {
        this.promiseField = getValue(42);
        print("Stored promise in class field");
        return new Int(1);
    }

    public function async getResult(): Promise<Int> {
        Int value = await this.promiseField;
        print("Retrieved value from field: " + value.toString());
        return value;
    }
}

function async testPromiseAsField(): Promise<Int> {
    print("--- Test 3: Promise as Class Field ---");
    PromiseContainer container = new PromiseContainer();
    await container.storePromise();
    Int result = await container.getResult();
    print("Final result from field: " + result.toString());
    return result;
}

// Test 4: Promise in collections
function async testPromiseInCollections(): Promise<Int> {
    print("--- Test 4: Promise in Collections ---");

    // Create a ArrayList to store promises
    ArrayList<Promise<Int>> promises = new ArrayList<Promise<Int>>();

    // Add multiple promises
    promises.add(getValue(1));
    promises.add(getValue(2));
    promises.add(getValue(3));
    print("Added 3 promises to ArrayList");

    // Await each promise
    int total = 0;
    for (int i = 0; i < promises.size(); i = i + 1) {
        Promise<Int> p = promises.get(i);
        Int value = await p;
        total = total + value.getValue();
    }
    print("Sum of all promises: " + new Int(total).toString());
    return new Int(total);
}

// Test 5: Promise as function parameter
function async processPromise(Promise<Int> p): Promise<Int> {
    Int value = await p;
    return new Int(value.getValue() * 2);
}

function async testPromiseAsParameter(): Promise<Int> {
    print("--- Test 5: Promise as Parameter ---");
    Promise<Int> p = getValue(7);
    Int result = await processPromise(p);
    print("Result from parameter test: " + result.toString());
    return result;
}

// Test 6: Promise return without await
function async wrapperReturningPromise(): Promise<Int> {
    Promise<Int> p = getValue(100);
    return await p;
}

function async testPromiseReturn(): Promise<Int> {
    print("--- Test 6: Promise Return Without Await ---");
    Int result = await wrapperReturningPromise();
    print("Result from wrapper: " + result.toString());
    return result;
}

// Test 7: Awaiting same Promise multiple times
function async testMultipleAwaits(): Promise<Int> {
    print("--- Test 7: Multiple Awaits on Same Promise ---");
    Promise<Int> p = getValue(50);

    Int v1 = await p;
    print("First await: " + v1.toString());

    Int v2 = await p;
    print("Second await: " + v2.toString());

    Int v3 = await p;
    print("Third await: " + v3.toString());

    return v3;
}

// Test 8: Promise in conditional/ternary
function async testPromiseInConditional(): Promise<Int> {
    print("--- Test 8: Promise in Conditional ---");
    bool condition = true;

    Promise<Int> p = condition ? getValue(111) : getValue(222);
    Int result = await p;
    print("Conditional promise result: " + result.toString());

    condition = false;
    p = condition ? getValue(111) : getValue(222);
    result = await p;
    print("Conditional promise result (false): " + result.toString());

    return result;
}

// Test 9: Promise with generic classes
class GenericContainer<T> {
    public T value;
}

function async testPromiseWithGenerics(): Promise<Int> {
    print("--- Test 9: Promise with Generic Classes ---");

    // Create container that holds a Promise
    GenericContainer<Promise<Int>> container = new GenericContainer<Promise<Int>>();
    container.value = getValue(999);
    print("Stored promise in generic container");

    Int result = await container.value;
    print("Result from generic container: " + result.toString());

    return result;
}

// Test 10: Chaining async operations with stored Promises
function async testPromiseChaining(): Promise<Int> {
    print("--- Test 10: Promise Chaining ---");

    Promise<Int> p1 = getValue(5);
    Int v1 = await p1;

    Promise<Int> p2 = getValue(v1.getValue() * 2);
    Int v2 = await p2;

    Promise<Int> p3 = getValue(v2.getValue() + 10);
    Int v3 = await p3;

    print("Chained result: " + v3.toString());

    return v3;
}

// Main test runner
function async main(): Promise<Int> {
    print("=== Comprehensive Promise Edge Cases Test ===");
    print("");

    await testPromiseReassignment();
    print("");

    await testNullPromise();
    print("");

    await testPromiseAsField();
    print("");

    await testPromiseInCollections();
    print("");

    
    await testPromiseAsParameter();
    print("");

    await testPromiseReturn();
    print("");

    await testMultipleAwaits();
    print("");

    await testPromiseInConditional();
    print("");

    await testPromiseWithGenerics();
    print("");

    Int finalResult = await testPromiseChaining();
    print("");

    print("=== All Edge Case Tests Completed ===");

    return finalResult;
}

// Run the tests
main();
