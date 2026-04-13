// Test async methods in interfaces
import * from "../../lib/primitives/Bool.mt";
class Result {
    int value;

    public constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

// Interface with async methods
interface DataProvider {
    function async fetchData(int id): Promise<Result>;
    function async saveData(Result data): Promise<Bool>;
}

// Implementation of async interface methods
class DatabaseProvider implements DataProvider {
    int callCount;

    public constructor() {
        this.callCount = 0;
    }

    // Implement async interface method
    public function async fetchData(int id): Promise<Result> {
        this.callCount = this.callCount + 1;
        return new Result(id * 10);
    }

    // Implement async interface method
    public function async saveData(Result data): Promise<Bool> {
        this.callCount = this.callCount + 1;
        print("Saving data with value: " + data.getValue());
        return new Bool(true);
    }

    public function getCallCount(): int {
        return this.callCount;
    }
}

print("=== Async Interface Methods Test ===");

// Main function to run test
function async main(): Promise<void> {
    DatabaseProvider provider = new DatabaseProvider();

    // Test async interface method - fetch
    Result data = await provider.fetchData(5);
    print("Fetched value: " + data.getValue());

    // Test async interface method - save
    Bool saved = await provider.saveData(data);
    print("Data saved: " + saved.toString());
    print("Total calls: " + provider.getCallCount());

}

main();
