// Test lambda lifecycle stress scenarios
// Tests many lambdas, rapid creation/destruction, and memory management

interface Worker {
    function work(int input): int;
}

interface WorkerFactory {
    function createWorker(int id): Worker;
}

interface TaskManager {
    function executeTasks(WorkerFactory factory, int taskCount): void;
}

class StressTaskManager implements TaskManager {
    public function executeTasks(WorkerFactory factory, int taskCount): void {
        print("Executing " + taskCount + " tasks...");

        for (int i = 0; i < taskCount; i++) {
            Worker worker = factory.createWorker(i);
            int result = worker.work(i * 10);
            print("Task " + i + " result: " + result);

            // Worker should be eligible for cleanup after this iteration
        }

        print("All tasks completed");
    }
}

class LambdaWorkerFactory implements WorkerFactory {
    public function createWorker(int id): Worker {
        // Create a new lambda for each worker
        // This tests rapid lambda creation and cleanup
        if (id % 4 == 0) {
            return input -> input + id;
        } else if (id % 4 == 1) {
            return input -> input * id;
        } else if (id % 4 == 2) {
            return input -> {
                if (input > 50) {
                    return input - id;
                } else {
                    return input + id;
                }
            };
        } else {
            return input -> {
                int temp = input + id;
                return temp * 2;
            };
        }
    }
}

function createMassLambdas(): void {
    print("=== Mass lambda creation test ===");

    Worker[] workers = new Worker[10];

    // Create many lambdas rapidly
    for (int i = 0; i < 10; i++) {
        int factor = i + 1;
        // MYT-215: capture a snapshot of the loop counter rather than `i`.
        int snapI = i;
        workers[i] = x -> x * factor + snapI;
    }

    // Use all lambdas
    for (int i = 0; i < 10; i++) {
        int result = workers[i].work(5);
        print("Worker " + i + " result: " + result);
    }

    // workers array goes out of scope, lambdas should be cleaned up
}

function testLambdaChaining(): void {
    print("=== Lambda chaining test ===");

    // Create a chain of lambdas that call each other
    Worker stage1 = x -> x + 10;
    Worker stage2 = x -> stage1.work(x) * 2;
    Worker stage3 = x -> stage2.work(x) - 5;
    Worker stage4 = x -> stage3.work(x) / 3;

    int input = 15;
    int result1 = stage1.work(input);
    int result2 = stage2.work(input);
    int result3 = stage3.work(input);
    int result4 = stage4.work(input);

    print("Chain results: " + result1 + ", " + result2 + ", " + result3 + ", " + result4);
}

function main(): void {
    print("Testing lambda lifecycle stress scenarios...");

    // Test 1: Rapid creation and destruction
    print("=== Test 1: Rapid lambda lifecycle ===");
    StressTaskManager manager = new StressTaskManager();
    LambdaWorkerFactory factory = new LambdaWorkerFactory();

    manager.executeTasks(factory, 15);

    // Test 2: Mass lambda creation
    createMassLambdas();

    // Test 3: Lambda chaining and interdependencies
    testLambdaChaining();

    // Test 4: Repeated factory pattern
    print("=== Test 4: Repeated factory usage ===");
    for (int round = 0; round < 3; round++) {
        print("Round " + round + ":");
        manager.executeTasks(factory, 5);
    }

    // Test 5: Lambda memory pressure simulation
    print("=== Test 5: Memory pressure simulation ===");
    Worker[] bigArray = new Worker[50];
    for (int i = 0; i < 50; i++) {
        int multiplier = i % 7 + 1;
        // MYT-215: snapshot the loop counter before capturing.
        int snapI = i;
        bigArray[i] = x -> x * multiplier + (snapI * 3);
    }

    // Use only some of them
    for (int i = 0; i < 50; i += 5) {
        int result = bigArray[i].work(i);
        print("BigArray[" + i + "] result: " + result);
    }

    print("Lambda stress test completed!");
}

main();