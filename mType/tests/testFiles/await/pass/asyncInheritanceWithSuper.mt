// Test async methods in inheritance with super calls

class Result {
    string message;
    int value;

    public constructor(string msg, int val) {
        this.message = msg;
        this.value = val;
    }

    public function getMessage(): string {
        return this.message;
    }

    public function getValue(): int {
        return this.value;
    }
}

print("=== Async Inheritance with Super Calls Test ===");

// Base class with async methods
class BaseService {
    string name;

    public constructor(string n) {
        this.name = n;
    }

    // Async method in base class
    public function async initialize(): Promise<Result> {
        print("BaseService initializing: " + this.name);
        Result r = new Result("Base initialized", 1);
        return r;
    }

    // Another async method in base class
    public function async process(int value): Promise<Result> {
        print("BaseService processing: " + value);
        int result = value * 2;
        Result r = new Result("Base processed", result);
        return r;
    }

    public function getName(): string {
        return this.name;
    }
}

// Derived class calling super async methods
class DerivedService extends BaseService {
    int callCount;

    public constructor(string n): super(n) {
        this.callCount = 0;
    }

    // Override async method and call super
    public function async initialize(): Promise<Result> {
        print("DerivedService calling super.initialize()");

        // Await super's async method
        Result baseResult = await super.initialize();
        print("Super returned: " + baseResult.getMessage());

        this.callCount = this.callCount + 1;

        // Return our own result
        Result derivedResult = new Result("Derived initialized", 10);
        return derivedResult;
    }

    // Override async method with super call
    public function async process(int value): Promise<Result> {
        print("DerivedService calling super.process()");

        // Await super's async method
        Result baseResult = await super.process(value);
        print("Super processed, result: " + baseResult.getValue());

        this.callCount = this.callCount + 1;

        // Do additional processing
        int finalValue = baseResult.getValue() + 100;
        Result derivedResult = new Result("Derived processed", finalValue);
        return derivedResult;
    }

    public function getCallCount(): int {
        return this.callCount;
    }
}

// Further derived class
class AdvancedService extends DerivedService {
    public constructor(string n):super(n) {
    }

    // Override async method and call super (which calls its super)
    public function async initialize(): Promise<Result> {
        print("AdvancedService calling super.initialize()");

        // This awaits DerivedService.initialize(), which awaits BaseService.initialize()
        Result derivedResult = await super.initialize();
        print("Derived returned: " + derivedResult.getMessage());

        // Return our own result
        Result advancedResult = new Result("Advanced initialized", 100);
        return advancedResult;
    }

    // Chain multiple super calls
    public function async complexProcess(int value): Promise<Result> {
        print("AdvancedService complex processing");

        // First super call
        Result r1 = await super.process(value);
        print("First super result: " + r1.getValue());

        // Second super call with different value
        Result r2 = await super.process(r1.getValue());
        print("Second super result: " + r2.getValue());

        // Combine results
        int finalValue = r1.getValue() + r2.getValue();
        Result finalResult = new Result("Advanced processed", finalValue);
        return finalResult;
    }
}

// Main function to run tests
function async main(): Promise<void> {
    // Test 1: Basic async super call
    print("\n--- Test 1: Basic Super Call ---");
    DerivedService service1 = new DerivedService("Service1");
    Result init1 = await service1.initialize();
    print("Final result: " + init1.getMessage() + " = " + init1.getValue());

    // Test 2: Async super call with parameters
    print("\n--- Test 2: Super Call with Parameters ---");
    Result proc1 = await service1.process(5);
    print("Final result: " + proc1.getMessage() + " = " + proc1.getValue());
    print("Call count: " + service1.getCallCount());

    // Test 3: Multi-level inheritance async super calls
    print("\n--- Test 3: Multi-level Inheritance ---");
    AdvancedService service2 = new AdvancedService("AdvancedService");
    Result init2 = await service2.initialize();
    print("Final result: " + init2.getMessage() + " = " + init2.getValue());

    // Test 4: Complex chained super calls
    print("\n--- Test 4: Chained Super Calls ---");
    Result complex = await service2.complexProcess(3);
    print("Final result: " + complex.getMessage() + " = " + complex.getValue());

    print("\nAll async inheritance tests completed");
}

main();
