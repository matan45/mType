// Test loops with dynamic class creation in namespaces
class Worker {
        int workerId;
        string task;
        static int totalWorkers = 0;

        public constructor(int id, string taskName) {
            workerId = id;
            task = taskName;
            totalWorkers = totalWorkers + 1;
        }

        public function work(): int {
            return workerId * 10; // Simulate work output
        }

        public static function getTotalWorkers(): int {
            return totalWorkers;
        }
}

function createWorkerBatch(int count): int {
    int totalOutput = 0;
    
    for (int i = 0; i < count; i++) {
        Worker worker = new Worker(i + 1, "Task" + i);
        int output = worker.work();
        totalOutput = totalOutput + output;
        
        // Nested loop for processing
        for (int j = 0; j < 3; j++) {
            if (output > 50 && j == 1) {
                totalOutput = totalOutput + 5; // Bonus
                break;
            }
        }
    }
    
    return totalOutput;
}

// Test loop-based class creation
int batchOutput1 = createWorkerBatch(5);
int batchOutput2 = createWorkerBatch(3);

print(batchOutput1);
print(batchOutput2);
print(Worker::getTotalWorkers());

// Test nested loops with namespace access
final int ITERATIONS = 4;

function nestedLoopTest(): int {
    int result = 0;
    
    for (int i = 0; i < ITERATIONS; i++) {
        Worker worker = new Worker(i + 100, "NestedTask");
        
        for (int j = 0; j < 2; j++) {
            result = result + worker.work();
            
            if (result > 500) {
                break;
            }
        }
    }
    
    return result;
}

int nestedResult = nestedLoopTest();
print(nestedResult);
print(Worker::getTotalWorkers());