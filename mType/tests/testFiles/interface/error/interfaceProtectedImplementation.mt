// Test protected implementation of interface method (should error)
// @Throw

interface Processor {
    func process(data: String): void;
}

class DataProcessor implements Processor {
    // Error: Interface methods must be public
    protected func process(data: String): void {
        print("Processing: " + data);
    }
}

var processor = new DataProcessor();
processor.process("test");  // Should fail
