// Test: Abstract async methods with optimization
// Verifies abstract async methods are preserved during optimization
import * from "../../lib/primitives/Int.mt";

abstract class AsyncService {
    protected string serviceName;

    public constructor(string name) {
        this.serviceName = name;
    }

    // Abstract async method
    abstract function async fetchData(): Promise<Int>;

    public function getServiceName(): string {
        // Dead code after return
        return this.serviceName;
        print("This should be removed");  // Should be removed
    }

    public function isReady(): bool {
        // Constant folding
        if (2 + 2 == 4) {  // Should fold to if(true)
            return true;
        } else {
            return false;  // Dead branch
        }
    }
}

class DataService extends AsyncService {
    private int value;

    public constructor(string name, int v) : super(name) {
        this.value = v;
    }

    // Implement abstract async method
    public function async fetchData(): Promise<Int> {
        print("Fetching data from " + this.serviceName);
        // Dead code after return
        return new Int(this.value * 2);
        print("Dead code after return");  // Should be removed
    }
}

// Test execution
DataService service = new DataService("MyService", 42);
print("Service: " + service.getServiceName());
print("Ready: " + service.isReady());
Promise<Int> result = service.fetchData();

print("Test Complete!");
