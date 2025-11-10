// Integration Test 02: Async Exception Handling with Inheritance
// Tests: Async/await + Try-catch-finally + Inheritance + @Override + @Throw annotation

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/exceptions/Exception.mt";

// Exception hierarchy
class BaseException extends Exception{
    

    constructor(string msg): super(msg) {
       
    }

   
}

class ValidationException extends BaseException {
    constructor(string msg): super(msg) {}
}

class ProcessingException extends BaseException {
    constructor(string msg): super(msg) {}
}

// Base async processor with exception handling
abstract class AsyncProcessor {
    protected int successCount;
    protected int errorCount;

    constructor() {
        this.successCount = 0;
        this.errorCount = 0;
    }

    @Throw(BaseException)
    abstract function async processData(int value): Promise<Int>;

    public function getStats(): void {
        print("Success: " + this.successCount + ", Errors: " + this.errorCount);
    }
}

// Concrete implementation with override
class ValidatingProcessor extends AsyncProcessor {
    private int threshold;

    constructor(int thresh) : super() {
        this.threshold = thresh;
    }

    @Override
    @Throw(ValidationException, ProcessingException)
    public function async processData(int value): Promise<Int> {
        try {
            print("Processing value: " + value);

            // Validation check
            if (value < 0) {
                ValidationException ex = new ValidationException("Negative value not allowed");
                throw ex;
            }

            // Processing check
            if (value > this.threshold) {
                ProcessingException ex = new ProcessingException("Value exceeds threshold");
                throw ex;
            }

            this.successCount = this.successCount + 1;
            return new Int(value * 2);

        } catch (ValidationException e) {
            this.errorCount = this.errorCount + 1;
            print("Caught ValidationException: " + e.getMessage());
            return new Int(-1);
        } catch (ProcessingException e) {
            this.errorCount = this.errorCount + 1;
            print("Caught ProcessingException: " + e.getMessage());
            return new Int(-2);
        } finally {
            print("Finally block executed for value: " + value);
        }
    }
}

// Another derived class with different behavior
class LoggingProcessor extends AsyncProcessor {
    constructor() : super() {}

    @Override
    @Throw(BaseException)
    public function async processData(int value): Promise<Int> {
        try {
            print("Logging: Processing " + value);

            if (value == 0) {
                BaseException ex = new BaseException("Zero value encountered");
                throw ex;
            }

            this.successCount = this.successCount + 1;
            return new Int(value + 100);
        } catch (BaseException e) {
            print("Caught BaseException: " + e.getMessage());
            this.errorCount = this.errorCount + 1;
            return new Int(0);
        } finally {
            print("Logging complete");
        }
    }
}

// Test async exception handling
function async testAsyncProcessor(AsyncProcessor processor, int testValue): Promise<Int> {
    Int result = await processor.processData(testValue);
    return result;
}

// Main test execution
function async main(): Promise<void> {
    print("=== Test 02: Async Exception Handling with Inheritance ===");

    // Test 1: ValidatingProcessor with various inputs
    ValidatingProcessor validator = new ValidatingProcessor(50);
    print("--- Testing ValidatingProcessor ---");

    Int r1 = await testAsyncProcessor(validator, 10);
    print("Result 1: " + r1.getValue());

    Int r2 = await testAsyncProcessor(validator, -5);
    print("Result 2: " + r2.getValue());

    Int r3 = await testAsyncProcessor(validator, 100);
    print("Result 3: " + r3.getValue());

    validator.getStats();

    // Test 2: LoggingProcessor with various inputs
    print("--- Testing LoggingProcessor ---");
    LoggingProcessor logger = new LoggingProcessor();

    Int r4 = await testAsyncProcessor(logger, 25);
    print("Result 4: " + r4.getValue());

    Int r5 = await testAsyncProcessor(logger, 0);
    print("Result 5: " + r5.getValue());

    logger.getStats();

    // Test 3: Polymorphic reference
    print("--- Testing Polymorphism ---");
    AsyncProcessor polyRef = new ValidatingProcessor(30);
    Int r6 = await polyRef.processData(20);
    print("Polymorphic result: " + r6.getValue());

    print("=== Test 02 Complete ===");
    return null;
}

main();
