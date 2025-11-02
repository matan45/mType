// Test: Type safety in async/await and control flow
// Tests type checking with asynchronous operations and promises

// Simulated async operations for testing
class Promise<T> {
    private T value;
    private bool resolved;

    public constructor() {
        this.resolved = false;
    }

    public void resolve(T val) {
        this.value = val;
        this.resolved = true;
    }

    public T getValue(): T {
        return this.value;
    }

    public bool isResolved(): bool {
        return this.resolved;
    }
}

// Test 1: Basic async type checking
print("Test 1: Basic async type checking");

class AsyncOperation {
    public Promise<int> computeAsync(int x): Promise<int> {
        Promise<int> promise = new Promise<int>();
        int result = x * 2;
        promise.resolve(result);
        return promise;
    }

    public Promise<string> formatAsync(int num): Promise<string> {
        Promise<string> promise = new Promise<string>();
        string result = "Result: " + num;
        promise.resolve(result);
        return promise;
    }
}

AsyncOperation asyncOp = new AsyncOperation();
Promise<int> intPromise = asyncOp.computeAsync(21);
int asyncResult = intPromise.getValue();
print("Async computation: " + asyncResult);

Promise<string> stringPromise = asyncOp.formatAsync(asyncResult);
string formatted = stringPromise.getValue();
print(formatted);

// Test 2: Async operations in loops
print("\nTest 2: Async operations in loops");

class AsyncProcessor {
    public Promise<int>[] processArray(int[] values, int size): Promise<int>[] {
        Promise<int>[] promises = new Promise<int>[size];
        int i = 0;
        while (i < size) {
            Promise<int> p = new Promise<int>();
            p.resolve(values[i] * 3);
            promises[i] = p;
            i = i + 1;
        }
        return promises;
    }

    public int sumPromises(Promise<int>[] promises, int size): int {
        int total = 0;
        int i = 0;
        while (i < size) {
            total = total + promises[i].getValue();
            i = i + 1;
        }
        return total;
    }
}

AsyncProcessor processor = new AsyncProcessor();
int[] values = new int[5];
int i = 0;
while (i < 5) {
    values[i] = i + 1;
    i = i + 1;
}

Promise<int>[] promises = processor.processArray(values, 5);
int sum = processor.sumPromises(promises, 5);
print("Sum of async results: " + sum);

// Test 3: Conditional async execution
print("\nTest 3: Conditional async execution");

class ConditionalAsync {
    public Promise<string> processConditional(int value): Promise<string> {
        Promise<string> promise = new Promise<string>();
        string result;

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
Promise<string> p1 = condAsync.processConditional(-5);
Promise<string> p2 = condAsync.processConditional(0);
Promise<string> p3 = condAsync.processConditional(10);

print(p1.getValue());
print(p2.getValue());
print(p3.getValue());

// Test 4: Async type inference with generics
print("\nTest 4: Async with generic types");

class AsyncContainer<T> {
    private Promise<T> promise;
    private bool hasPromise;

    public constructor() {
        this.hasPromise = false;
    }

    public void setPromise(Promise<T> p) {
        this.promise = p;
        this.hasPromise = true;
    }

    public T await(): T {
        if (this.hasPromise) {
            return this.promise.getValue();
        }
        // Should not reach here in valid code
        return this.promise.getValue();
    }
}

AsyncContainer<int> intContainer = new AsyncContainer<int>();
Promise<int> intProm = new Promise<int>();
intProm.resolve(99);
intContainer.setPromise(intProm);
print("Awaited int: " + intContainer.await());

AsyncContainer<string> stringContainer = new AsyncContainer<string>();
Promise<string> strProm = new Promise<string>();
strProm.resolve("Hello Async");
stringContainer.setPromise(strProm);
print("Awaited string: " + stringContainer.await());

// Test 5: Async chain operations
print("\nTest 5: Async chain operations");

class AsyncChain {
    public Promise<int> step1(int x): Promise<int> {
        Promise<int> p = new Promise<int>();
        p.resolve(x + 10);
        return p;
    }

    public Promise<int> step2(int x): Promise<int> {
        Promise<int> p = new Promise<int>();
        p.resolve(x * 2);
        return p;
    }

    public Promise<int> step3(int x): Promise<int> {
        Promise<int> p = new Promise<int>();
        p.resolve(x - 5);
        return p;
    }

    public int executeChain(int initial): int {
        Promise<int> p1 = this.step1(initial);
        int result1 = p1.getValue();
        print("After step1: " + result1);

        Promise<int> p2 = this.step2(result1);
        int result2 = p2.getValue();
        print("After step2: " + result2);

        Promise<int> p3 = this.step3(result2);
        int result3 = p3.getValue();
        print("After step3: " + result3);

        return result3;
    }
}

AsyncChain chain = new AsyncChain();
int chainResult = chain.executeChain(5);
print("Final chain result: " + chainResult);

// Test 6: Async error handling pattern
print("\nTest 6: Async error handling");

class AsyncResult<T> {
    private T value;
    private bool hasValue;
    private string error;
    private bool hasError;

    public constructor() {
        this.hasValue = false;
        this.hasError = false;
    }

    public void succeed(T val) {
        this.value = val;
        this.hasValue = true;
    }

    public void fail(string err) {
        this.error = err;
        this.hasError = true;
    }

    public bool isSuccess(): bool {
        return this.hasValue;
    }

    public bool isError(): bool {
        return this.hasError;
    }

    public T getValue(): T {
        return this.value;
    }

    public string getError(): string {
        return this.error;
    }
}

class SafeAsyncOp {
    public AsyncResult<int> safeDivide(int a, int b): AsyncResult<int> {
        AsyncResult<int> result = new AsyncResult<int>();
        if (b == 0) {
            result.fail("Division by zero");
        } else {
            result.succeed(a / b);
        }
        return result;
    }

    public void handleResult(AsyncResult<int> result) {
        if (result.isSuccess()) {
            print("Success: " + result.getValue());
        } else {
            if (result.isError()) {
                print("Error: " + result.getError());
            }
        }
    }
}

SafeAsyncOp safeOp = new SafeAsyncOp();
AsyncResult<int> res1 = safeOp.safeDivide(20, 4);
safeOp.handleResult(res1);

AsyncResult<int> res2 = safeOp.safeDivide(10, 0);
safeOp.handleResult(res2);

// Test 7: Async parallel operations simulation
print("\nTest 7: Async parallel operations");

class ParallelAsync {
    public Promise<int>[] createTasks(int count): Promise<int>[] {
        Promise<int>[] tasks = new Promise<int>[count];
        int i = 0;
        while (i < count) {
            Promise<int> p = new Promise<int>();
            p.resolve((i + 1) * 10);
            tasks[i] = p;
            i = i + 1;
        }
        return tasks;
    }

    public void awaitAll(Promise<int>[] tasks, int count) {
        int i = 0;
        while (i < count) {
            int result = tasks[i].getValue();
            print("Task " + i + " result: " + result);
            i = i + 1;
        }
    }
}

ParallelAsync parallel = new ParallelAsync();
Promise<int>[] tasks = parallel.createTasks(4);
parallel.awaitAll(tasks, 4);

// Test 8: Type-safe async callbacks
print("\nTest 8: Type-safe async callbacks");

class Callback<T> {
    private T result;
    private bool executed;

    public constructor() {
        this.executed = false;
    }

    public void execute(T value) {
        this.result = value;
        this.executed = true;
    }

    public T getResult(): T {
        return this.result;
    }

    public bool hasExecuted(): bool {
        return this.executed;
    }
}

class AsyncWithCallback {
    public void processWithCallback(int value, Callback<string> callback) {
        string result = "Processed: " + value;
        callback.execute(result);
    }
}

AsyncWithCallback asyncCb = new AsyncWithCallback();
Callback<string> callback = new Callback<string>();
asyncCb.processWithCallback(42, callback);

if (callback.hasExecuted()) {
    print(callback.getResult());
}

print("\nAsync type safety tests completed");
