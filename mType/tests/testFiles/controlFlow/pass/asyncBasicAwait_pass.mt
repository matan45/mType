// Test basic async/await control flow
// Validates simple async function calls and await expressions

class Result {
    int value;
    string message;

    public constructor(int v, string msg) {
        this.value = v;
        this.message = msg;
    }

    public function getValue(): int {
        return this.value;
    }

    public function getMessage(): string {
        return this.message;
    }
}

print("=== Basic Async Await Test ===");

// Simple async function
function async fetchData(): Promise<Result> {
    print("Fetching data...");
    Result r = new Result(42, "Data fetched");
    return r;
}

// Async function that awaits another
function async processData(): Promise<Result> {
    print("Processing data...");
    Result data = await fetchData();
    print("Data received: " + data.getMessage());
    Result processed = new Result(data.getValue() * 2, "Data processed");
    return processed;
}

// Main async function
function async main(): Promise<void> {
    print("Starting async operation");

    Result result = await processData();
    print("Final value: " + result.getValue());
    print("Final message: " + result.getMessage());

    print("Async operation complete");
}

main();
