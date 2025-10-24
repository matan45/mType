// Test: Abstract async method
// Abstract methods CAN be async - the implementing class must provide an async implementation
import * from "../../lib/primitives/Int.mt";
abstract class AsyncService {
    abstract function async processData(): Promise<Int>;

    public function describe(): void {
        print("I am an async service");
    }
}

class DataProcessor extends AsyncService {
    private int value;

    constructor(int v) {
        value = v;
    }

    public function async processData(): Promise<Int> {
        print("Processing data asynchronously...");
        return value * 2;
    }
}

// Test the implementation
DataProcessor processor = new DataProcessor(21);
processor.describe();
Promise<Int> result = processor.processData();
print("Processing complete");
