// Test lambda casting to interface type
// @Script
import * from "../../lib/primitives/String.mt";
interface Runnable {
    function run(): void;
}

interface Supplier<T> {
    function get(): T;
}

class SimpleRunnable implements Runnable {
    public function run(): void {
        print("Task is running");
    }
}

class StringSupplier implements Supplier<String> {
    public function get(): String {
        return new String("Result from supplier");
    }
}

class TaskExecutor {
    public function execute(Runnable task): void {
        print("Executing task...");
        task.run();
        print("Task completed");
    }

    public function <T> executeWithResult(Supplier<T> supplier): T {
        print("Executing supplier...");
        T result = supplier.get();
        print("Supplier completed");
        return result;
    }
}

TaskExecutor executor = new TaskExecutor();

// Create runnable
SimpleRunnable task = new SimpleRunnable();
executor.execute(task);

// Create supplier
StringSupplier supplier = new StringSupplier();
String result = executor.executeWithResult<String>(supplier);
print(result.toString());
