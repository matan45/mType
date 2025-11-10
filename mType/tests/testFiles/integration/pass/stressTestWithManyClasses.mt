// Stress test with many class instances

    class Worker {
        int id;
        string task;
        static int totalWorkers = 0;
        
        constructor(int workerId, string taskName) {
            id = workerId;
            task = taskName;
            totalWorkers = totalWorkers + 1;
        }

        public function doWork(): int {
            return id * 10;
        }
        
        public static function getTotalWorkers(): int {
            return totalWorkers;
        }
    }
    
    public function createManyWorkers(): int {
        int totalWork = 0;
        final int NUM_WORKERS = 20;
        
        for (int i = 0; i < NUM_WORKERS; i++) {
            Worker worker = new Worker(i, "Task" + i);
            int work = worker.doWork();
            totalWork = totalWork + work;
        }
        
        return totalWork;
    }


// Run stress test
int totalWork = createManyWorkers();
print(totalWork);
print(Worker::getTotalWorkers());