// Test passing null when interface expected
// @Script

interface Processor {
    func process(data: String): void;
}

class DataProcessor implements Processor {
    func process(data: String): void {
        print("Processing: " + data);
    }
}

class Service {
    func execute(processor: Processor): void {
        if (processor == null) {
            print("No processor provided");
            return;
        }
        processor.process("test data");
    }
}

var service = new Service();

// Pass null - should be handled gracefully
service.execute(null);

// Pass actual processor
service.execute(new DataProcessor());
