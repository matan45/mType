// Test: Async operations with casting and exception handling integration
// @Script

interface AsyncProcessor {
    function async process() : Promise<String>;
}

interface ErrorHandler {
    function handleError(String error) : String;
}

class BaseTask implements AsyncProcessor, ErrorHandler {
    protected Int taskId;
    protected String taskName;
    protected Bool shouldFail;

    constructor(Int id, String name, Bool fail) {
        this.taskId = id;
        this.taskName = name;
        this.shouldFail = fail;
    }

    function async process() : Promise<String> {
        await delay(5);
        if (this.shouldFail) {
            throw "Task " + this.taskId.toString() + " failed";
        }
        return "Task " + this.taskId.toString() + " completed";
    }

    function handleError(String error) : String {
        return "Handled: " + error;
    }

    function getTaskId() : Int {
        return this.taskId;
    }

    function getName() : String {
        return this.taskName;
    }

    function setShouldFail(Bool fail) : void {
        this.shouldFail = fail;
    }
}

class DataTask extends BaseTask {
    private String data;

    constructor(Int id, String name, Bool fail, String d) {
        super(id, name, fail);
        this.data = d;
    }

    function async process() : Promise<String> {
        await delay(5);
        if (this.shouldFail) {
            throw "DataTask " + this.taskId.toString() + " failed: invalid data";
        }
        return "DataTask " + this.taskId.toString() + " processed: " + this.data;
    }

    function getData() : String {
        return this.data;
    }
}

class ComputeTask extends BaseTask {
    private Int value;
    private Int multiplier;

    constructor(Int id, String name, Bool fail, Int val, Int mult) {
        super(id, name, fail);
        this.value = val;
        this.multiplier = mult;
    }

    function async process() : Promise<String> {
        await delay(5);
        if (this.shouldFail) {
            throw "ComputeTask " + this.taskId.toString() + " failed: computation error";
        }
        Int result = this.value * this.multiplier;
        return "ComputeTask " + this.taskId.toString() + " result: " + result.toString();
    }

    function async computeAsync() : Promise<Int> {
        await delay(5);
        return this.value * this.multiplier;
    }
}

function async delay(Int ms) : Promise<void> {
    // Simulated delay
}

function async processWithCasting<T extends BaseTask>(T task) : Promise<String> {
    try {
        print("Processing task: " + task.getName());

        // Try casting to DataTask
        DataTask? dataTask = task as DataTask;
        if (dataTask != null) {
            print("  Type: DataTask, Data: " + dataTask.getData());
        }

        // Try casting to ComputeTask
        ComputeTask? computeTask = task as ComputeTask;
        if (computeTask != null) {
            print("  Type: ComputeTask");
            Int computed = await computeTask.computeAsync();
            print("  Computed value: " + computed.toString());
        }

        // Cast to AsyncProcessor and process
        AsyncProcessor processor = task as AsyncProcessor;
        String result = await processor.process();
        print("  Result: " + result);
        return result;

    } catch (String e) {
        print("  Error caught: " + e);

        // Cast to ErrorHandler and handle
        ErrorHandler handler = task as ErrorHandler;
        String handled = handler.handleError(e);
        print("  " + handled);
        return handled;
    }
}

function async processBatch(BaseTask[] tasks) : Promise<Int> {
    Int successCount = 0;
    Int i = 0;

    while (i < tasks.length()) {
        try {
            BaseTask task = tasks[i];
            String result = await processWithCasting<BaseTask>(task);

            if (!result.contains("Handled")) {
                successCount = successCount + 1;
            }

        } catch (String e) {
            print("Unexpected error processing task " + i.toString() + ": " + e);
        }

        i = i + 1;
    }

    return successCount;
}

function async retryTask<T extends BaseTask>(T task, Int maxAttempts) : Promise<Bool> {
    Int attempt = 1;

    while (attempt <= maxAttempts) {
        try {
            print("Attempt " + attempt.toString() + " for task " + task.getTaskId().toString());

            AsyncProcessor processor = task as AsyncProcessor;
            String result = await processor.process();
            print("Success: " + result);
            return true;

        } catch (String e) {
            print("Attempt " + attempt.toString() + " failed: " + e);

            if (attempt == maxAttempts) {
                print("All attempts exhausted");
                ErrorHandler handler = task as ErrorHandler;
                String finalResult = handler.handleError(e);
                print(finalResult);
                return false;
            }

            attempt = attempt + 1;
            await delay(5);
        }
    }

    return false;
}

function async chainTasks(BaseTask task1, BaseTask task2) : Promise<String> {
    try {
        print("Chaining tasks...");

        String result1 = await processWithCasting<BaseTask>(task1);

        // Only proceed to second task if first succeeded
        if (!result1.contains("Handled")) {
            String result2 = await processWithCasting<BaseTask>(task2);
            return result1 + " -> " + result2;
        }

        return result1;

    } catch (String e) {
        return "Chain failed: " + e;
    }
}

function async main() : Promise<void> {
    print("=== Async Cast Exception Test ===");

    // Test 1: Process different task types
    print("\n--- Test 1: Different Task Types ---");
    BaseTask[] tasks = new BaseTask[4];
    tasks[0] = new BaseTask(1, "Base1", false);
    tasks[1] = new DataTask(2, "Data1", false, "sample data");
    tasks[2] = new ComputeTask(3, "Compute1", false, 10, 5);
    tasks[3] = new DataTask(4, "Data2", true, "error data");

    Int count = await processBatch(tasks);
    print("Successfully processed: " + count.toString() + " tasks");

    // Test 2: Retry mechanism
    print("\n--- Test 2: Retry Mechanism ---");
    ComputeTask retryTask = new ComputeTask(5, "Retry1", true, 7, 3);
    Bool success = await retryTask<ComputeTask>(retryTask, 3);
    print("Retry result: " + success.toString());

    // Test 3: Task chaining
    print("\n--- Test 3: Task Chaining ---");
    DataTask successTask = new DataTask(6, "Chain1", false, "chain data");
    ComputeTask failTask = new ComputeTask(7, "Chain2", true, 8, 2);

    String chainResult = await chainTasks(successTask, failTask);
    print("Chain result: " + chainResult);

    // Test 4: Polymorphic async operations
    print("\n--- Test 4: Polymorphic Operations ---");
    BaseTask task = new ComputeTask(8, "Poly1", false, 15, 4);

    // Upcast to interface
    AsyncProcessor processor = task as AsyncProcessor;
    String result = await processor.process();
    print("Polymorphic result: " + result);

    // Downcast back
    ComputeTask? computeTask = processor as ComputeTask;
    if (computeTask != null) {
        Int value = await computeTask.computeAsync();
        print("Downcast successful, computed: " + value.toString());
    }

    // Test 5: Exception propagation in async context
    print("\n--- Test 5: Exception Propagation ---");
    DataTask errorTask = new DataTask(9, "Error1", true, "bad data");

    try {
        String res = await processWithCasting<DataTask>(errorTask);
        print("Final result: " + res);
    } catch (String e) {
        print("Caught at top level: " + e);
    }

    print("\n=== All tests completed ===");
}
