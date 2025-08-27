// Stress test with many class instances
namespace stressTest {
    class Worker {
        int id;
        string task;
        static int totalWorkers = 0;
        
        constructor(int workerId, string taskName) {
            id = workerId;
            task = taskName;
            totalWorkers = totalWorkers + 1;
        }
        
        function doWork(): int {
            return id * 10;
        }
        
        static function getTotalWorkers(): int {
            return totalWorkers;
        }
    }
    
    function createManyWorkers(): int {
        int totalWork = 0;
        final int NUM_WORKERS = 20;
        
        for (int i = 0; i < NUM_WORKERS; i++) {
            Worker worker = new Worker(i, "Task" + toString(i));
            int work = worker.doWork();
            totalWork = totalWork + work;
        }
        
        return totalWork;
    }
}

// Run stress test
int totalWork = stressTest::createManyWorkers();
print(totalWork);
print(stressTest::Worker::getTotalWorkers());