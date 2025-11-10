// Test: Partial failure - some operations succeed, some fail
// Expected: Should handle mixed success/failure scenarios
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Custom exceptions
class ProcessingException extends Exception {
    public int itemId;

    public constructor(string msg, int id): super(msg) {
        itemId = id;
    }

    public function getItemId(): int {
        return itemId;
    }
}

// Result wrapper for partial operations
class OperationResult<T> {
    public bool success;
    public T value;
    public Exception error;

    public constructor(bool s, T v, Exception e) {
        success = s;
        value = v;
        error = e;
    }

    public function isSuccess(): bool {
        return success;
    }

    public function getValue(): T {
        return value;
    }

    public function getError(): Exception {
        return error;
    }
}

// Batch result tracking
class BatchResult {
    public int totalCount;
    public int successCount;
    public int failureCount;

    public constructor() {
        totalCount = 0;
        successCount = 0;
        failureCount = 0;
    }

    public function recordSuccess(): void {
        totalCount = totalCount + 1;
        successCount = successCount + 1;
    }

    public function recordFailure(): void {
        totalCount = totalCount + 1;
        failureCount = failureCount + 1;
    }

    public function getSuccessRate(): int {
        if (totalCount == 0) {
            return 0;
        }
        return (successCount * 100) / totalCount;
    }

    public function toString(): string {
        return "Total: " + totalCount + ", Success: " + successCount + ", Failed: " + failureCount + " (Rate: " + getSuccessRate() + "%)";
    }
}

// Test 1: Process array with partial failures
print("=== Test 1: Batch processing with failures ===");

function processItem(int item): string {
    print("Processing item: " + item);

    // Simulate failures for even numbers
    if (item % 2 == 0) {
        throw new ProcessingException("Item " + item + " failed validation", item);
    }

    return "Processed-" + item;
}

BatchResult batch1 = new BatchResult();
int[] items = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

int i = 0;
while (i < 10) {
    try {
        string result = processItem(items[i]);
        batch1.recordSuccess();
        print("Success: " + result);
    } catch (ProcessingException e) {
        batch1.recordFailure();
        print("Failed: Item " + e.getItemId() + " - " + e.getMessage());
    }
    i = i + 1;
}

print("Batch results: " + batch1.toString());

// Test 2: Collect all results (success and failure)
print("\n=== Test 2: Collect all results ===");

OperationResult<String>[] results = new OperationResult<String>[5];

int j = 0;
while (j < 5) {
    int itemId = j + 1;
    try {
        string processed = processItem(itemId);
        results[j] = new OperationResult<String>(true, processed, null);
    } catch (ProcessingException e) {
        results[j] = new OperationResult<String>(false, null, e);
    }
    j = j + 1;
}

// Process results
print("Processing results:");
int k = 0;
while (k < 5) {
    OperationResult<String> result = results[k];
    if (result.isSuccess()) {
        print("Item " + k + ": SUCCESS - " + result.getValue());
    } else {
        print("Item " + k + ": FAILURE - " + result.getError().getMessage());
    }
    k = k + 1;
}

// Test 3: Continue on error pattern
print("\n=== Test 3: Continue processing on error ===");

class Task {
    public int id;
    public string name;

    public constructor(int taskId, string taskName) {
        id = taskId;
        name = taskName;
    }

    public function execute(): string {
        print("Executing task: " + name);

        if (id == 2 || id == 4) {
            throw new Exception("Task " + id + " encountered an error");
        }

        return "Task " + id + " completed";
    }
}

Task[] tasks = [
    new Task(1, "Initialize"),
    new Task(2, "LoadData"),
    new Task(3, "Process"),
    new Task(4, "Validate"),
    new Task(5, "Finalize")
];

BatchResult taskResults = new BatchResult();
int m = 0;
while (m < 5) {
    try {
        string outcome = tasks[m].execute();
        taskResults.recordSuccess();
        print("Result: " + outcome);
    } catch (Exception e) {
        taskResults.recordFailure();
        print("Error: " + e.getMessage());
        print("Continuing to next task...");
    }
    m = m + 1;
}

print("Task execution summary: " + taskResults.toString());

// Test 4: Partial success threshold
print("\n=== Test 4: Success threshold check ===");

function executeWithThreshold(int successThreshold): bool {
    BatchResult batch = new BatchResult();
    int n = 1;

    while (n <= 10) {
        try {
            processItem(n);
            batch.recordSuccess();
        } catch (ProcessingException e) {
            batch.recordFailure();
        }
        n = n + 1;
    }

    int rate = batch.getSuccessRate();
    print("Success rate: " + rate + "%, Threshold: " + successThreshold + "%");

    return rate >= successThreshold;
}

bool meetsThreshold = executeWithThreshold(40);
if (meetsThreshold) {
    print("Threshold met - operation acceptable");
} else {
    print("Threshold not met - operation unacceptable");
}

// Test 5: Rollback on critical failure
print("\n=== Test 5: Partial rollback pattern ===");

class Transaction {
    public int id;
    public bool committed;

    public constructor(int txId) {
        id = txId;
        committed = false;
    }

    public function commit(): void {
        print("Committing transaction " + id);

        if (id == 3) {
            throw new Exception("Transaction " + id + " commit failed");
        }

        committed = true;
    }

    public function rollback(): void {
        if (committed) {
            print("Rolling back transaction " + id);
            committed = false;
        }
    }

    public function isCommitted(): bool {
        return committed;
    }
}

Transaction[] transactions = [
    new Transaction(1),
    new Transaction(2),
    new Transaction(3),
    new Transaction(4)
];

int p = 0;
bool allSucceeded = true;

while (p < 4) {
    try {
        transactions[p].commit();
    } catch (Exception e) {
        print("Failed: " + e.getMessage());
        print("Rolling back all committed transactions");

        // Rollback all previously committed
        int q = 0;
        while (q < p) {
            transactions[q].rollback();
            q = q + 1;
        }

        allSucceeded = false;
        break;
    }
    p = p + 1;
}

if (allSucceeded) {
    print("All transactions committed successfully");
} else {
    print("Transaction batch failed - all rolled back");
}

// Verify final state
print("Final transaction states:");
int r = 0;
while (r < 4) {
    string state = transactions[r].isCommitted() ? "COMMITTED" : "ROLLED_BACK";
    print("Transaction " + transactions[r].id + ": " + state);
    r = r + 1;
}

print("\nPartial failure test completed!");
