// Test handling of larger data structures
final int LARGE_SIZE = 100;

class DataProcessor {
    int processedCount;
    final int BATCH_SIZE = 10;
    
    constructor() {
        processedCount = 0;
    }
    
    function processLargeDataset(): int {
        int totalSum = 0;
        
        // Simulate processing large dataset
        for (int i = 0; i < LARGE_SIZE; i++) {
            // Process in batches
            if (i % BATCH_SIZE == 0) {
                // Batch processing logic
                int batchSum = 0;
                for (int j = 0; j < BATCH_SIZE && (i + j) < LARGE_SIZE; j++) {
                    batchSum = batchSum + (i + j);
                }
                totalSum += batchSum;
                processedCount += BATCH_SIZE;
            }
            
            // Occasional complex operation
            if (i % 20 == 0) {
                int complexResult = 0;
                for (int k = 0; k < 5; k++) {
                    complexResult += (i * k);
                }
                totalSum += complexResult;
            }
        }
        
        return totalSum;
    }
    
    function getProcessedCount(): int {
        return processedCount;
    }
}

function analyzeResults(int result, int processedCount): string {
    if (result > 100000) {
        return "Large result: " + result + " from " + processedCount + " items";
    } else if (result > 10000) {
        return "Medium result: " + result + " from " + processedCount + " items";
    } else {
        return "Small result: " + result + " from " + processedCount + " items";
    }
}

// Test large data processing
DataProcessor processor = new DataProcessor();
int result = processor.processLargeDataset();
int processed = processor.getProcessedCount();

print(result);
print(processed);

string analysis = analyzeResults(result, processed);
print(analysis);

// Test multiple processors
DataProcessor processor2 = new DataProcessor();
int result2 = processor2.processLargeDataset();
print(result2);