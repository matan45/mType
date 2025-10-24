// Test: Abstract class with dead code elimination
// Verifies that abstract methods are preserved while dead code is removed

abstract class DataProcessor {
    protected string name;

    public constructor(string n) {
        this.name = n;
    }

    // Abstract method - should be preserved
    abstract function process(): void;

    // Concrete method with dead code
    public function getName(): string {
        return this.name;
        print("Dead code after return");  // Should be removed
    }

    // Method with unreachable if branch
    public function validate(): bool {
        if (true) {
            return true;
        } else {
            print("Dead branch");  // Should be removed
            return false;
        }
    }
}

class TextProcessor extends DataProcessor {
    private string text;

    public constructor(string n, string t) : super(n) {
        this.text = t;
    }

    // Implementation of abstract method
    public function process(): void {
        print("Processing: " + this.text);
        return;
        print("Dead after return in process");  // Should be removed
    }
}

// Test execution
TextProcessor processor = new TextProcessor("TextProc", "Hello World");
print("Name: " + processor.getName());
processor.process();
print("Valid: " + processor.validate());

print("Test Complete!");
