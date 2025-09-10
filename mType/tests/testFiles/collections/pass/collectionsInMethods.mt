// Test collections in method contexts (simpler version without class fields)

class DataProcessor {
    DataProcessor() {
        // Simple constructor
    }
    
    Array<int> processNumbers(Array<int> input) {
        Array<int> result = new Array<int>();
        for (int num : input) {
            result.add(num * 2);
        }
        return result;
    }
    
    Map<string, int> createScoreMap() {
        Map<string, int> scores = new Map<string, int>();
        scores.put("Alice", 95);
        scores.put("Bob", 87);
        scores.put("Charlie", 92);
        return scores;
    }
    
    void printArray(Array<int> numbers) {
        print("Numbers:");
        for (int num : numbers) {
            print(num);
        }
    }
    
    void printMap(Map<string, int> data) {
        print("Map values:");
        for (string value : data) {
            print(value);
        }
    }
    
    int sumArray(Array<int> numbers) {
        int total = 0;
        for (int num : numbers) {
            total = total + num;
        }
        return total;
    }
}

// Test collections as parameters and return types
print("Testing collections in methods:");

DataProcessor processor = new DataProcessor();

// Test method with array parameter and return
Array<int> original = [1, 2, 3, 4, 5];
Array<int> doubled = processor.processNumbers(original);

print("Original array:");
processor.printArray(original);

print("Doubled array:");
processor.printArray(doubled);

// Test method returning map
Map<string, int> scoreMap = processor.createScoreMap();
processor.printMap(scoreMap);

// Test method with collection parameter
print("Sum of original:", processor.sumArray(original));
print("Sum of doubled:", processor.sumArray(doubled));

// Test with direct collection literals
Array<int> testNumbers = [10, 20, 30];
print("Test sum:", processor.sumArray(testNumbers));

print("Collections in methods test passed");