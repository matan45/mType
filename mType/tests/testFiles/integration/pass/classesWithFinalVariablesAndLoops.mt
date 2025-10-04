class Configuration {
    public final int MAX_ITERATIONS = 100;
    public final string CONFIG_NAME = "Default";
    public static int totalConfigs = 0;

    public constructor() {
        totalConfigs = totalConfigs + 1;
    }

    public function runSimulation(): int {
        final int BATCH_SIZE = 10;
        int total = 0;
        
        for (int i = 0; i < MAX_ITERATIONS; i++) {
            if (i % BATCH_SIZE == 0) {
                total = total + BATCH_SIZE;
                if (total > 50) {
                    break;
                }
            }
        }
        
        return total;
    }
    
    public function processWithWhile(): int {
        final int TARGET = 42;
        int current = 0;
        int iterations = 0;
        
        while (current < TARGET && iterations < MAX_ITERATIONS) {
            current = current + 3;
            iterations = iterations + 1;
            
            if (current % 10 == 0) {
                continue;
            }
        }
        
        return current;
    }
}


    final int NUM_TESTS = 5;
    
    function runBatchTests(): int {
        int totalResults = 0;
        
        for (int test = 0; test < NUM_TESTS; test++) {
            Configuration config = new Configuration();
            int simResult = config.runSimulation();
            int whileResult = config.processWithWhile();
            
            totalResults = totalResults + simResult + whileResult;
            
            // Test nested loops with classes
            for (int inner = 0; inner < 3; inner++) {
                if (simResult > 30 && inner == 1) {
                    break;
                }
                totalResults = totalResults + 1;
            }
        }
        
        return totalResults;
    }


// Execute complex test
int result = runBatchTests();
print(result);
print(Configuration::totalConfigs);

// Test final variables in loops - edge case
final int LOOP_LIMIT = 10;
int sum = 0;
for (int i = 0; i < LOOP_LIMIT; i++) {
    int LOCAL_MULTIPLIER = i * 2;
    sum += LOCAL_MULTIPLIER;
}
print(sum);