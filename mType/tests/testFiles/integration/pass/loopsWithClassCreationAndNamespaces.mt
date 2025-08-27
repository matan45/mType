// Test loops with dynamic class creation in namespaces
namespace factory {
    class Worker {
        int workerId;
        string task;
        static int totalWorkers = 0;
        
        constructor(int id, string taskName) {
            workerId = id;
            task = taskName;
            totalWorkers = totalWorkers + 1;
        }
        
        function work(): int {
            return workerId * 10; // Simulate work output
        }
        
        static function getTotalWorkers(): int {
            return totalWorkers;
        }
    }
    
    namespace production {
        function createWorkerBatch(int count): int {
            int totalOutput = 0;
            
            for (int i = 0; i < count; i++) {
                factory::Worker worker = new factory::Worker(i + 1, "Task" + toString(i));
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
    }
}

// Test loop-based class creation
int batchOutput1 = factory::production::createWorkerBatch(5);
int batchOutput2 = factory::production::createWorkerBatch(3);

print(batchOutput1);
print(batchOutput2);
print(factory::Worker::getTotalWorkers());

// Test nested loops with namespace access
namespace testLoop {
    final int ITERATIONS = 4;
    
    function nestedLoopTest(): int {
        int result = 0;
        
        for (int i = 0; i < ITERATIONS; i++) {
            factory::Worker worker = new factory::Worker(i + 100, "NestedTask");
            
            for (int j = 0; j < 2; j++) {
                result = result + worker.work();
                
                if (result > 500) {
                    break;
                }
            }
        }
        
        return result;
    }
}

int nestedResult = testLoop::nestedLoopTest();
print(nestedResult);
print(factory::Worker::getTotalWorkers());