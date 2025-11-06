// Test: Type safety in async/await and control flow
// Tests type checking with asynchronous operations and promises
import * from "../../../lib/primitives/Int.mt";

// Simulated async operations for testing
class Promise<T> {
    private T value;
    private Bool resolved;

    public constructor() {
        this.resolved = false;
    }

    public void resolve(T val) {
        this.value = val;
        this.resolved = true;
    }

    public T getValue() {
        return this.value;
    }

    public Bool isResolved() {
        return this.resolved;
    }
}

// Test 1: Basic async type checking
print("Test 1: Basic async type checking");

class AsyncOperation {
    public Promise<Int> computeAsync(Int x) {
        Promise<Int> promise = new Promise<Int>();
        Int result = x * 2;
        promise.resolve(new Int(result));
        return promise;
    }

    public Promise<String> formatAsync(Int num) {
        Promise<String> promise = new Promise<String>();
        String result = "Result: " + num;
        promise.resolve(result);
        return promise;
    }
}

AsyncOperation asyncOp = new AsyncOperation();
Promise<Int> intPromise = asyncOp.computeAsync(21);
Int asyncResult = intPromise.getValue();
print("Async computation: " + asyncResult.getValue());

Promise<String> stringPromise = asyncOp.formatAsync(asyncResult.getValue());
String formatted = stringPromise.getValue();
print(formatted);

// Test 2: Async operations in loops
print("\nTest 2: Async operations in loops");

class AsyncProcessor {
    public Promise<Int>[] processArray(Int[] values, Int size) {
        Promise<Int>[] promises = new Promise<Int>[size];
        Int i = 0;
        while (i < size) {
            Promise<Int> p = new Promise<Int>();
            p.resolve(new Int(values[i] * 3));
            promises[i] = p;
            i = i + 1;
        }
        return promises;
    }

    public Int sumPromises(Promise<Int>[] promises, Int size) {
        Int total = 0;
        Int i = 0;
        while (i < size) {
            total = total + promises[i].getValue().getValue();
            i = i + 1;
        }
        return total;
    }
}

AsyncProcessor processor = new AsyncProcessor();
Int[] values = new Int[5];
Int i = 0;
while (i < 5) {
    values[i] = i + 1;
    i = i + 1;
}

Promise<Int>[] promises = processor.processArray(values, 5);
Int sum = processor.sumPromises(promises, 5);
print("Sum of async results: " + sum);

// Test 3: Conditional async execution
print("\nTest 3: Conditional async execution");

class ConditionalAsync {
    public Promise<String> processConditional(Int value) {
        Promise<String> promise = new Promise<String>();
        String result;

        if (value < 0) {
            result = "Negative value: " + value;
        } else {
            if (value == 0) {
                result = "Zero value";
            } else {
                result = "Positive value: " + value;
            }
        }

        promise.resolve(result);
        return promise;
    }
}

ConditionalAsync condAsync = new ConditionalAsync();
Promise<String> p1 = condAsync.processConditional(-5);
Promise<String> p2 = condAsync.processConditional(0);
Promise<String> p3 = condAsync.processConditional(10);

print(p1.getValue());
print(p2.getValue());
print(p3.getValue());

// Test 4: Async type inference with generics
print("\nTest 4: Async with generic types");

class AsyncContainer<T> {
    private Promise<T> promise;
    private Bool hasPromise;

    public constructor() {
        this.hasPromise = false;
    }

    public void setPromise(Promise<T> p) {
        this.promise = p;
        this.hasPromise = true;
    }

    public T await() {
        if (this.hasPromise) {
            return this.promise.getValue();
        }
        // Should not reach here in valid code
        return this.promise.getValue();
    }
}

AsyncContainer<Int> intContainer = new AsyncContainer<Int>();
Promise<Int> intProm = new Promise<Int>();
intProm.resolve(99);
intContainer.setPromise(intProm);
print("Awaited int: " + intContainer.await());

AsyncContainer<String> stringContainer = new AsyncContainer<String>();
Promise<String> strProm = new Promise<String>();
strProm.resolve("Hello Async");
stringContainer.setPromise(strProm);
print("Awaited string: " + stringContainer.await());

// Test 5: Async chain operations
print("\nTest 5: Async chain operations");

class AsyncChain {
    public Promise<Int> step1(Int x) {
        Promise<Int> p = new Promise<Int>();
        p.resolve(new Int(x + 10));
        return p;
    }

    public Promise<Int> step2(Int x) {
        Promise<Int> p = new Promise<Int>();
        p.resolve(new Int(x * 2));
        return p;
    }

    public Promise<Int> step3(Int x) {
        Promise<Int> p = new Promise<Int>();
        p.resolve(new Int(x - 5));
        return p;
    }

    public Int executeChain(Int initial) {
        Promise<Int> p1 = this.step1(initial);
        Int result1 = p1.getValue();
        print("After step1: " + result1.getValue());

        Promise<Int> p2 = this.step2(result1.getValue());
        Int result2 = p2.getValue();
        print("After step2: " + result2.getValue());

        Promise<Int> p3 = this.step3(result2.getValue());
        Int result3 = p3.getValue();
        print("After step3: " + result3.getValue());

        return result3.getValue();
    }
}

AsyncChain chain = new AsyncChain();
Int chainResult = chain.executeChain(5);
print("Final chain result: " + chainResult);

// Test 6: Async error handling pattern
print("\nTest 6: Async error handling");

class AsyncResult<T> {
    private T value;
    private Bool hasValue;
    private String error;
    private Bool hasError;

    public constructor() {
        this.hasValue = false;
        this.hasError = false;
    }

    public void succeed(T val) {
        this.value = val;
        this.hasValue = true;
    }

    public void fail(String err) {
        this.error = err;
        this.hasError = true;
    }

    public Bool isSuccess() {
        return this.hasValue;
    }

    public Bool isError() {
        return this.hasError;
    }

    public T getValue() {
        return this.value;
    }

    public String getError() {
        return this.error;
    }
}

class SafeAsyncOp {
    public AsyncResult<Int> safeDivide(Int a, Int b) {
        AsyncResult<Int> result = new AsyncResult<Int>();
        if (b == 0) {
            result.fail("Division by zero");
        } else {
            result.succeed(a / b);
        }
        return result;
    }

    public void handleResult(AsyncResult<Int> result) {
        if (result.isSuccess()) {
            print("Success: " + result.getValue().getValue());
        } else {
            if (result.isError()) {
                print("Error: " + result.getError());
            }
        }
    }
}

SafeAsyncOp safeOp = new SafeAsyncOp();
AsyncResult<Int> res1 = safeOp.safeDivide(20, 4);
safeOp.handleResult(res1);

AsyncResult<Int> res2 = safeOp.safeDivide(10, 0);
safeOp.handleResult(res2);

// Test 7: Async parallel operations simulation
print("\nTest 7: Async parallel operations");

class ParallelAsync {
    public Promise<Int>[] createTasks(Int count) {
        Promise<Int>[] tasks = new Promise<Int>[count];
        Int i = 0;
        while (i < count) {
            Promise<Int> p = new Promise<Int>();
            p.resolve(new Int((i + 1) * 10));
            tasks[i] = p;
            i = i + 1;
        }
        return tasks;
    }

    public void awaitAll(Promise<Int>[] tasks, Int count) {
        Int i = 0;
        while (i < count) {
            Int result = tasks[i].getValue();
            print("Task " + i + " result: " + result.getValue());
            i = i + 1;
        }
    }
}

ParallelAsync parallel = new ParallelAsync();
Promise<Int>[] tasks = parallel.createTasks(4);
parallel.awaitAll(tasks, 4);

// Test 8: Type-safe async callbacks
print("\nTest 8: Type-safe async callbacks");

class Callback<T> {
    private T result;
    private Bool executed;

    public constructor() {
        this.executed = false;
    }

    public void execute(T value) {
        this.result = value;
        this.executed = true;
    }

    public T getResult() {
        return this.result;
    }

    public Bool hasExecuted() {
        return this.executed;
    }
}

class AsyncWithCallback {
    public void processWithCallback(Int value, Callback<String> callback) {
        String result = "Processed: " + value;
        callback.execute(result);
    }
}

AsyncWithCallback asyncCb = new AsyncWithCallback();
Callback<String> callback = new Callback<String>();
asyncCb.processWithCallback(42, callback);

if (callback.hasExecuted()) {
    print(callback.getResult());
}

print("\nAsync type safety tests completed");
