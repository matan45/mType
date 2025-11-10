// Test: @Throw annotation inheritance in class hierarchy
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";

class DataException extends Exception {
    public constructor(string msg) : super(msg) {
    }
}

class ValidationException extends DataException {
    public constructor(string msg) : super(msg) {
    }
}

class ParseException extends DataException {
    public constructor(string msg) : super(msg) {
    }
}

// Base class with @Throw annotation
class DataProcessor {
    @Throw(DataException)
    public function processData(string data): string {
        print("Processing base data: " + data);
        if (data == "error") {
            throw new DataException("Data processing failed");
        }
        return "Processed: " + data;
    }
}

// Derived class can throw same or more specific exceptions
class ValidatingProcessor extends DataProcessor {
    @Throw(ValidationException)
    @Override
    public function processData(string data): string {
        print("Validating data: " + data);
        if (data == "invalid") {
            throw new ValidationException("Validation failed");
        }
        return super.processData(data);
    }
}

// Another derived class with different specific exception
class ParsingProcessor extends DataProcessor {
    @Throw(ParseException)
    @Override
    public function processData(string data): string {
        print("Parsing data: " + data);
        if (data == "parse-error") {
            throw new ParseException("Parse error occurred");
        }
        return super.processData(data);
    }
}

// Test case 1: Base class processing
function testBaseProcessor(): void {
    DataProcessor processor = new DataProcessor();
    try {
        string result = processor.processData("test data");
        print("Result: " + result);
    } catch (DataException e) {
        print("Caught DataException: " + e.getMessage());
    }
}

// Test case 2: Validating processor
function testValidatingProcessor(): void {
    ValidatingProcessor processor = new ValidatingProcessor();
    try {
        string result = processor.processData("invalid");
        print("Should not reach here");
    } catch (ValidationException e) {
        print("Caught ValidationException: " + e.getMessage());
    }
}

// Test case 3: Parsing processor
function testParsingProcessor(): void {
    ParsingProcessor processor = new ParsingProcessor();
    try {
        string result = processor.processData("parse-error");
        print("Should not reach here");
    } catch (ParseException e) {
        print("Caught ParseException: " + e.getMessage());
    }
}

// Test case 4: Polymorphic behavior - catching base exception
function testPolymorphicCatch(): void {
    DataProcessor processor = new ValidatingProcessor();
    try {
        string result = processor.processData("invalid");
        print("Should not reach here");
    } catch (DataException e) {
        print("Caught base DataException polymorphically: " + e.getMessage());
    }
}

print("=== Test 1: Base processor ===");
testBaseProcessor();

print("");
print("=== Test 2: Validating processor ===");
testValidatingProcessor();

print("");
print("=== Test 3: Parsing processor ===");
testParsingProcessor();

print("");
print("=== Test 4: Polymorphic catch ===");
testPolymorphicCatch();

print("");
print("Throw inheritance test passed!");
