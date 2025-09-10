// Test simple nesting with collections inside functions

function testArrayInFunction(): void {
    Array<int> outerNumbers = [1, 2, 3];
    Array<string> outerNames = ["Alice", "Bob"];
    
    print("Outer numbers:");
    for (int num : outerNumbers) {
        print(num);
    }
    
    print("Outer names:");
    for (string name : outerNames) {
        print(name);
    }
    
    // Simulate nested behavior with multiple collections
    Map<string, int> dataMap = new Map<string, int>();
    dataMap.put("count", outerNumbers.size());
    dataMap.put("total", 6);
    print("Data from map:");
    for (int value : dataMap) {
        print(value);
    }
}

function testMultipleCollections(): void {
    Array<int> numbers1 = [10, 20];
    Array<int> numbers2 = [30, 40];
    Array<string> labels = ["first", "second"];
    
    print("Multiple collections:");
    
    int totalSum = 0;
    for (int num : numbers1) {
        totalSum = totalSum + num;
    }
    for (int num : numbers2) {
        totalSum = totalSum + num;
    }
    
    print("Total sum:", totalSum);
    
    for (string label : labels) {
        print("Label:", label);
    }
}

// Test nested-like behavior
print("Testing nested collections:");

testArrayInFunction();
testMultipleCollections();

print("Nested collections test passed");