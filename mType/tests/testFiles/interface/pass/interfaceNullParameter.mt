// Test passing null when interface expected
// @Script

interface Processor {
    function process(string data): void;
}

class DataProcessor implements Processor {
    public function process(string data): void {
        print("Processing: " + data);
    }
}

class Service {
    public function execute(Processor processor): void {
        if (processor == null) {
            print("No processor provided");
            return;
        }
        processor.process("test data");
    }
}

Service service = new Service();

// Pass null - should be handled gracefully
service.execute(null);

// Pass actual processor
service.execute(new DataProcessor());
