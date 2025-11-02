// Test lambda casting to interface type
// @Script

interface Runnable {
    func run(): void;
}

interface Supplier<T> {
    func get(): T;
}

class LambdaRunnable implements Runnable {
    var fn: func(): void;

    func init(fn: func(): void) {
        this.fn = fn;
    }

    func run(): void {
        this.fn();
    }
}

class LambdaSupplier<T> implements Supplier<T> {
    var fn: func(): T;

    func init(fn: func(): T) {
        this.fn = fn;
    }

    func get(): T {
        return this.fn();
    }
}

class TaskExecutor {
    func execute(task: Runnable): void {
        print("Executing task...");
        task.run();
        print("Task completed");
    }

    func executeWithResult<T>(supplier: Supplier<T>): T {
        print("Executing supplier...");
        var result = supplier.get();
        print("Supplier completed");
        return result;
    }
}

var executor = new TaskExecutor();

// Create lambda runnable
var task = new LambdaRunnable(func(): void {
    print("Task is running");
});

executor.execute(task);

// Create lambda supplier
var supplier = new LambdaSupplier<String>(func(): String {
    return "Result from supplier";
});

var result = executor.executeWithResult<String>(supplier);
print(result);
