// Test collections as local variables and function parameters

// Function that takes array and returns sum
int calculateSum(Array<int> numbers) {
    int total = 0;
    for (int num : numbers) {
        total = total + num;
    }
    return total;
}

// Function that modifies array contents
void doubleValues(Array<int> numbers) {
    for (int i = 0; i < numbers.size(); i = i + 1) {
        int current = numbers.get(i);
        numbers.set(i, current * 2);
    }
}

// Function that works with maps
void printMapContents(Map<string, int> data) {
    print("Map contents:");
    for (string value : data) {
        print(value);
    }
}

// Function that creates and returns array
Array<string> createNameArray() {
    Array<string> names = new Array<string>();
    names.add("Alice");
    names.add("Bob");
    names.add("Charlie");
    return names;
}

// Function that creates and returns map
Map<string, string> createColorMap() {
    Map<string, string> colors = new Map<string, string>();
    colors.put("red", "#FF0000");
    colors.put("blue", "#0000FF");
    colors.put("green", "#00FF00");
    return colors;
}

// Test collection functions
print("Testing collections as function parameters and return types:");

// Test array parameter and for-each
Array<int> testNumbers = [1, 2, 3, 4, 5];
print("Original sum:", calculateSum(testNumbers));

// Test array modification via parameter
doubleValues(testNumbers);
print("After doubling:", calculateSum(testNumbers));

// Test function returning array
Array<string> names = createNameArray();
print("Names from function:");
for (string name : names) {
    print("- " + name);
}

// Test function returning map
Map<string, string> colorMap = createColorMap();
print("Colors from function:");
for (string color : colorMap) {
    print(color);
}

// Test map parameter
Map<string, int> scoreMap = {"Math": 95, "Science": 87, "History": 92};
printMapContents(scoreMap);

// Test with nested function calls
Array<int> newNumbers = [10, 20, 30];
print("Nested call result:", calculateSum(newNumbers));

print("Collections as function variables test passed");