// Test collections with functions (correct syntax)

function processNumbers(): void {
    Array<int> numbers = [1, 2, 3, 4, 5];
    print("Processing numbers:");
    
    int sum = 0;
    for (int num : numbers) {
        sum = sum + num;
        print("Number:", num);
    }
    print("Total sum:", sum);
}

function processStrings(): void {
    Array<string> names = ["Alice", "Bob", "Charlie"];
    print("Processing names:");
    
    for (string name : names) {
        print("Name:", name);
    }
    print("Name count:", names.size());
}

function processMap(): void {
    Map<string, int> grades = {"Math": 95, "Science": 87, "History": 92};
    print("Processing grades:");
    
    print("Subject count:", grades.size());
    for (int grade : grades) {
        print("Grade:", grade);
    }
    
    print("Has Math:", grades.containsKey("Math"));
    print("Has English:", grades.containsKey("English"));
}

function createAndReturnArray(): void {
    Array<int> data = new Array<int>();
    data.add(10);
    data.add(20);
    data.add(30);
    
    print("Created array:");
    for (int value : data) {
        print("Value:", value);
    }
}

// Test all functions
print("Testing collections with functions:");

processNumbers();
processStrings();
processMap();
createAndReturnArray();

print("Collections with functions test passed");