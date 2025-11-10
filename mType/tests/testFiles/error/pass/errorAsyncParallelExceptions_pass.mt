// Test: Multiple async exceptions in parallel
// Expected: Parallel async operations handle exceptions independently
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

class TaskException extends Exception {
    public int taskId;

    public constructor(string msg, int id): super(msg) {
        taskId = id;
    }

    public function getTaskId(): int {
        return taskId;
    }
}

// Task functions that may succeed or fail
function async task1(): Promise<String> {
    print("Task 1: Starting");
    print("Task 1: Completed successfully");
    return new String("Task1-OK");
}

function async task2(): Promise<String> {
    print("Task 2: Starting");
    print("Task 2: Throwing exception");
    TaskException e = new TaskException("Task 2 failed", 2);
    throw e;
    return new String("Task2-OK");
}

function async task3(): Promise<String> {
    print("Task 3: Starting");
    print("Task 3: Completed successfully");
    return new String("Task3-OK");
}

function async task4(): Promise<String> {
    print("Task 4: Starting");
    print("Task 4: Throwing exception");
    TaskException e = new TaskException("Task 4 failed", 4);
    throw e;
    return new String("Task4-OK");
}

// Test 1: Independent parallel tasks with individual error handling
function async independentTasks(): Promise<String> {
    print("Starting independent tasks");

    // Task 1 - should succeed
    String result1;
    try {
        Promise<String> p1 = task1();
        result1 = await p1;
        print("Task 1 result: " + result1.toString());
    } catch (TaskException e) {
        result1 = new String("Task1-Failed");
        print("Task 1 failed: " + e.getMessage());
    }

    // Task 2 - should fail
    String result2;
    try {
        Promise<String> p2 = task2();
        result2 = await p2;
        print("Task 2 result: " + result2.toString());
    } catch (TaskException e) {
        result2 = new String("Task2-Failed");
        print("Task 2 caught: " + e.getMessage() + " (ID: " + e.getTaskId() + ")");
    }

    // Task 3 - should succeed
    String result3;
    try {
        Promise<String> p3 = task3();
        result3 = await p3;
        print("Task 3 result: " + result3.toString());
    } catch (TaskException e) {
        result3 = new String("Task3-Failed");
        print("Task 3 failed: " + e.getMessage());
    }

    // Task 4 - should fail
    String result4;
    try {
        Promise<String> p4 = task4();
        result4 = await p4;
        print("Task 4 result: " + result4.toString());
    } catch (TaskException e) {
        result4 = new String("Task4-Failed");
        print("Task 4 caught: " + e.getMessage() + " (ID: " + e.getTaskId() + ")");
    }

    return new String("All tasks completed");
}

// Test 2: Parallel operations with aggregated results
class ResultCollector {
    public Int successCount;
    public Int failureCount;

    public constructor() {
        successCount = new Int(0);
        failureCount = new Int(0);
    }

    public function recordSuccess(): void {
        successCount = new Int(successCount.getValue() + 1);
    }

    public function recordFailure(): void {
        failureCount = new Int(failureCount.getValue() + 1);
    }

    public function report(): string {
        return "Success: " + successCount.toString() + ", Failures: " + failureCount.toString();
    }
}

function async parallelOperation(Int id, bool shouldFail): Promise<Int> {
    print("Operation " + id.toString() + " running");
    if (shouldFail) {
        Exception e = new Exception("Operation " + id.toString() + " error");
        throw e;
    }
    return id;
}

function async aggregatedResults(): Promise<String> {
    print("Starting aggregated parallel operations");
    ResultCollector collector = new ResultCollector();

    // Operation 1 - success
    try {
        Promise<Int> p1 = parallelOperation(new Int(1), false);
        Int r1 = await p1;
        print("Op 1 succeeded: " + r1.toString());
        collector.recordSuccess();
    } catch (Exception e) {
        print("Op 1 failed: " + e.getMessage());
        collector.recordFailure();
    }

    // Operation 2 - fail
    try {
        Promise<Int> p2 = parallelOperation(new Int(2), true);
        Int r2 = await p2;
        print("Op 2 succeeded: " + r2.toString());
        collector.recordSuccess();
    } catch (Exception e) {
        print("Op 2 failed: " + e.getMessage());
        collector.recordFailure();
    }

    // Operation 3 - success
    try {
        Promise<Int> p3 = parallelOperation(new Int(3), false);
        Int r3 = await p3;
        print("Op 3 succeeded: " + r3.toString());
        collector.recordSuccess();
    } catch (Exception e) {
        print("Op 3 failed: " + e.getMessage());
        collector.recordFailure();
    }

    // Operation 4 - fail
    try {
        Promise<Int> p4 = parallelOperation(new Int(4), true);
        Int r4 = await p4;
        print("Op 4 succeeded: " + r4.toString());
        collector.recordSuccess();
    } catch (Exception e) {
        print("Op 4 failed: " + e.getMessage());
        collector.recordFailure();
    }

    // Operation 5 - success
    try {
        Promise<Int> p5 = parallelOperation(new Int(5), false);
        Int r5 = await p5;
        print("Op 5 succeeded: " + r5.toString());
        collector.recordSuccess();
    } catch (Exception e) {
        print("Op 5 failed: " + e.getMessage());
        collector.recordFailure();
    }

    String report = collector.report();
    print("Final report: " + report);
    return new String("Aggregation complete");
}

// Test 3: First failure stops subsequent tasks
function async failFast(): Promise<String> {
    print("Starting fail-fast operations");
    try {
        Promise<Int> p1 = parallelOperation(new Int(1), false);
        Int r1 = await p1;
        print("Op 1 done: " + r1.toString());

        Promise<Int> p2 = parallelOperation(new Int(2), true);
        Int r2 = await p2;
        print("Op 2 done: " + r2.toString());

        Promise<Int> p3 = parallelOperation(new Int(3), false);
        Int r3 = await p3;
        print("Op 3 done: " + r3.toString());

        return new String("all completed");
    } catch (Exception e) {
        print("Fail-fast caught: " + e.getMessage());
        return new String("stopped early");
    }
}

// Test 4: Different exception types in parallel
class NetworkException extends Exception {
    public constructor(string msg): super(msg) {
    }
}

class ValidationException extends Exception {
    public constructor(string msg): super(msg) {
    }
}

function async networkTask(): Promise<String> {
    print("Network task running");
    NetworkException e = new NetworkException("Network error");
    throw e;
    return new String("network-ok");
}

function async validationTask(): Promise<String> {
    print("Validation task running");
    ValidationException e = new ValidationException("Validation error");
    throw e;
    return new String("validation-ok");
}

function async mixedExceptions(): Promise<String> {
    print("Starting mixed exception types");

    try {
        Promise<String> p1 = networkTask();
        String r1 = await p1;
        print("Network: " + r1.toString());
    } catch (NetworkException e) {
        print("Caught network exception: " + e.getMessage());
    }

    try {
        Promise<String> p2 = validationTask();
        String r2 = await p2;
        print("Validation: " + r2.toString());
    } catch (ValidationException e) {
        print("Caught validation exception: " + e.getMessage());
    }

    return new String("Mixed exceptions handled");
}

function async main(): Promise<void> {
    print("=== Test 1: Independent parallel tasks ===");
    Promise<String> p1 = independentTasks();
    String result1 = await p1;
    print("Result: " + result1.toString());

    print("");
    print("=== Test 2: Aggregated results ===");
    Promise<String> p2 = aggregatedResults();
    String result2 = await p2;
    print("Result: " + result2.toString());

    print("");
    print("=== Test 3: Fail-fast ===");
    Promise<String> p3 = failFast();
    String result3 = await p3;
    print("Result: " + result3.toString());

    print("");
    print("=== Test 4: Mixed exception types ===");
    Promise<String> p4 = mixedExceptions();
    String result4 = await p4;
    print("Result: " + result4.toString());

    print("");
    print("Parallel exception test completed");
    return null;
}

main();
