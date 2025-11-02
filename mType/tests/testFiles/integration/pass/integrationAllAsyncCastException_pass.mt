// Test: Async operations with casting and exception handling integration
// @Script

interface AsyncProcessor {
    async process() : Promise<String>;
}

interface ErrorHandler {
    handleError(error: String) : String;
}

class BaseTask : AsyncProcessor, ErrorHandler {
    protected taskId: Int;
    protected taskName: String;
    protected shouldFail: Bool;

    constructor(id: Int, name: String, fail: Bool) {
        this.taskId = id;
        this.taskName = name;
        this.shouldFail = fail;
    }

    async process() : Promise<String> {
        await delay(5);
        if (this.shouldFail) {
            throw "Task " + this.taskId.toString() + " failed";
        }
        return "Task " + this.taskId.toString() + " completed";
    }

    handleError(error: String) : String {
        return "Handled: " + error;
    }

    getTaskId() : Int {
        return this.taskId;
    }

    getName() : String {
        return this.taskName;
    }

    setShouldFail(fail: Bool) : Void {
        this.shouldFail = fail;
    }
}

class DataTask extends BaseTask {
    private data: String;

    constructor(id: Int, name: String, fail: Bool, d: String) {
        super(id, name, fail);
        this.data = d;
    }

    async process() : Promise<String> {
        await delay(5);
        if (this.shouldFail) {
            throw "DataTask " + this.taskId.toString() + " failed: invalid data";
        }
        return "DataTask " + this.taskId.toString() + " processed: " + this.data;
    }

    getData() : String {
        return this.data;
    }
}

class ComputeTask extends BaseTask {
    private value: Int;
    private multiplier: Int;

    constructor(id: Int, name: String, fail: Bool, val: Int, mult: Int) {
        super(id, name, fail);
        this.value = val;
        this.multiplier = mult;
    }

    async process() : Promise<String> {
        await delay(5);
        if (this.shouldFail) {
            throw "ComputeTask " + this.taskId.toString() + " failed: computation error";
        }
        let result: Int = this.value * this.multiplier;
        return "ComputeTask " + this.taskId.toString() + " result: " + result.toString();
    }

    async computeAsync() : Promise<Int> {
        await delay(5);
        return this.value * this.multiplier;
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async processWithCasting<T extends BaseTask>(task: T) : Promise<String> {
    try {
        print("Processing task: " + task.getName());

        // Try casting to DataTask
        let dataTask: DataTask? = task as DataTask;
        if (dataTask != null) {
            print("  Type: DataTask, Data: " + dataTask.getData());
        }

        // Try casting to ComputeTask
        let computeTask: ComputeTask? = task as ComputeTask;
        if (computeTask != null) {
            print("  Type: ComputeTask");
            let computed: Int = await computeTask.computeAsync();
            print("  Computed value: " + computed.toString());
        }

        // Cast to AsyncProcessor and process
        let processor: AsyncProcessor = task as AsyncProcessor;
        let result: String = await processor.process();
        print("  Result: " + result);
        return result;

    } catch (e: String) {
        print("  Error caught: " + e);

        // Cast to ErrorHandler and handle
        let handler: ErrorHandler = task as ErrorHandler;
        let handled: String = handler.handleError(e);
        print("  " + handled);
        return handled;
    }
}

async processBatch(tasks: BaseTask[]) : Promise<Int> {
    let successCount: Int = 0;
    let i: Int = 0;

    while (i < tasks.length()) {
        try {
            let task: BaseTask = tasks[i];
            let result: String = await processWithCasting<BaseTask>(task);

            if (!result.contains("Handled")) {
                successCount = successCount + 1;
            }

        } catch (e: String) {
            print("Unexpected error processing task " + i.toString() + ": " + e);
        }

        i = i + 1;
    }

    return successCount;
}

async retryTask<T extends BaseTask>(task: T, maxAttempts: Int) : Promise<Bool> {
    let attempt: Int = 1;

    while (attempt <= maxAttempts) {
        try {
            print("Attempt " + attempt.toString() + " for task " + task.getTaskId().toString());

            let processor: AsyncProcessor = task as AsyncProcessor;
            let result: String = await processor.process();
            print("Success: " + result);
            return true;

        } catch (e: String) {
            print("Attempt " + attempt.toString() + " failed: " + e);

            if (attempt == maxAttempts) {
                print("All attempts exhausted");
                let handler: ErrorHandler = task as ErrorHandler;
                let finalResult: String = handler.handleError(e);
                print(finalResult);
                return false;
            }

            attempt = attempt + 1;
            await delay(5);
        }
    }

    return false;
}

async chainTasks(task1: BaseTask, task2: BaseTask) : Promise<String> {
    try {
        print("Chaining tasks...");

        let result1: String = await processWithCasting<BaseTask>(task1);

        // Only proceed to second task if first succeeded
        if (!result1.contains("Handled")) {
            let result2: String = await processWithCasting<BaseTask>(task2);
            return result1 + " -> " + result2;
        }

        return result1;

    } catch (e: String) {
        return "Chain failed: " + e;
    }
}

async main() : Promise<Void> {
    print("=== Async Cast Exception Test ===");

    // Test 1: Process different task types
    print("\n--- Test 1: Different Task Types ---");
    let tasks: BaseTask[] = new BaseTask[4];
    tasks[0] = new BaseTask(1, "Base1", false);
    tasks[1] = new DataTask(2, "Data1", false, "sample data");
    tasks[2] = new ComputeTask(3, "Compute1", false, 10, 5);
    tasks[3] = new DataTask(4, "Data2", true, "error data");

    let count: Int = await processBatch(tasks);
    print("Successfully processed: " + count.toString() + " tasks");

    // Test 2: Retry mechanism
    print("\n--- Test 2: Retry Mechanism ---");
    let retryTask: ComputeTask = new ComputeTask(5, "Retry1", true, 7, 3);
    let success: Bool = await retryTask<ComputeTask>(retryTask, 3);
    print("Retry result: " + success.toString());

    // Test 3: Task chaining
    print("\n--- Test 3: Task Chaining ---");
    let successTask: DataTask = new DataTask(6, "Chain1", false, "chain data");
    let failTask: ComputeTask = new ComputeTask(7, "Chain2", true, 8, 2);

    let chainResult: String = await chainTasks(successTask, failTask);
    print("Chain result: " + chainResult);

    // Test 4: Polymorphic async operations
    print("\n--- Test 4: Polymorphic Operations ---");
    let task: BaseTask = new ComputeTask(8, "Poly1", false, 15, 4);

    // Upcast to interface
    let processor: AsyncProcessor = task as AsyncProcessor;
    let result: String = await processor.process();
    print("Polymorphic result: " + result);

    // Downcast back
    let computeTask: ComputeTask? = processor as ComputeTask;
    if (computeTask != null) {
        let value: Int = await computeTask.computeAsync();
        print("Downcast successful, computed: " + value.toString());
    }

    // Test 5: Exception propagation in async context
    print("\n--- Test 5: Exception Propagation ---");
    let errorTask: DataTask = new DataTask(9, "Error1", true, "bad data");

    try {
        let res: String = await processWithCasting<DataTask>(errorTask);
        print("Final result: " + res);
    } catch (e: String) {
        print("Caught at top level: " + e);
    }

    print("\n=== All tests completed ===");
}
