class Configuration {
    final int MAX_ITERATIONS = 100;
    final string CONFIG_NAME = "Default";
    static int totalConfigs = 0;
    
    constructor() {
        totalConfigs = totalConfigs + 1;
    }
    
    function runSimulation(): int {
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
    
    function processWithWhile(): int {
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

namespace testSuite {
    final int NUM_TESTS = 5;
    
    function runBatchTests(): int {
        int totalResults = 0;
        
        for (int test = 0; test < NUM_TESTS; test++) {
            Configuration config = Configuration();
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
}

// Execute complex test
int result = testSuite::runBatchTests();
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