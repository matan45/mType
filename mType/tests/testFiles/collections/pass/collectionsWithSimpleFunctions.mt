// Test collections with functions (avoiding generic parameters)

void processArray() {
    Array<int> numbers = [1, 2, 3, 4, 5];
    print("Processing array:");
    
    int sum = 0;
    for (int num : numbers) {
        sum = sum + num;
        print("Number:", num);
    }
    print("Sum:", sum);
}

void processMap() {
    Map<string, int> studentGrades = {"Alice": 95, "Bob": 87, "Charlie": 92};
    print("Processing map:");
    
    print("Map size:", studentGrades.size());
    for (int grade : studentGrades) {
        print("Grade:", grade);
    }
    
    print("Has Alice:", studentGrades.containsKey("Alice"));
    print("Has David:", studentGrades.containsKey("David"));
}

void testArrayOperations() {
    Array<string> names = new Array<string>();
    names.add("Alice");
    names.add("Bob");
    names.add("Charlie");
    
    print("Names array:");
    for (string name : names) {
        print("-", name);
    }
    
    print("First name:", names.get(0));
    print("Array size:", names.size());
}

void testMapOperations() {
    Map<string, string> colors = new Map<string, string>();
    colors.put("red", "#FF0000");
    colors.put("blue", "#0000FF");
    colors.put("green", "#00FF00");
    
    print("Color map:");
    for (string colorCode : colors) {
        print("Color:", colorCode);
    }
    
    print("Red color:", colors.get("red"));
}

// Simple class to test collections in class context
class DataHandler {
    DataHandler() {
        // Simple constructor
    }
    
    void handleNumbers() {
        Array<int> data = [10, 20, 30, 40, 50];
        print("Handling numbers in class:");
        
        for (int value : data) {
            print("Value:", value);
        }
    }
    
    void handleStrings() {
        Array<string> words = ["hello", "world", "test"];
        print("Handling strings in class:");
        
        for (string word : words) {
            print("Word:", word);
        }
    }
}

// Test all functions
print("Testing collections with simple functions:");

processArray();
processMap();
testArrayOperations();
testMapOperations();

DataHandler handler = new DataHandler();
handler.handleNumbers();
handler.handleStrings();

print("Collections with simple functions test passed");