// Test: Throwing generic exception instance
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Bool.mt";

// Generic exception with metadata
class MetadataException<T> extends Exception {
    public T metadata;

    public constructor(string msg, T meta) {
        super(msg);
        metadata = meta;
    }

    public function getMetadata(): T {
        return metadata;
    }
}

// Helper class for metadata
class ErrorContext {
    public int line;
    public string file;
    public bool critical;

    public constructor(int lineNum, string fileName, bool isCritical) {
        line = lineNum;
        file = fileName;
        critical = isCritical;
    }

    public function getLine(): int {
        return line;
    }

    public function getFile(): string {
        return file;
    }

    public function isCritical(): bool {
        return critical;
    }
}

// Function that throws generic exception with ErrorContext
function processFile(string filename): void {
    ErrorContext ctx = new ErrorContext(42, filename, true);
    throw new MetadataException<ErrorContext>("File processing error", ctx);
}

// Function that throws generic exception with Int
function divide(int a, int b): int {
    if (b == 0) {
        throw new MetadataException<Int>("Division by zero", new Int(b));
    }
    return a / b;
}

// Function that throws generic exception with String
function validateInput(string input): bool {
    if (input == "") {
        throw new MetadataException<String>("Empty input", new String("EMPTY_STRING"));
    }
    return true;
}

// Test throwing and catching different generic exceptions
print("Testing throw with ErrorContext metadata:");
try {
    processFile("data.txt");
} catch (MetadataException<ErrorContext> e) {
    print("Caught: " + e.getMessage());
    ErrorContext ctx = e.getMetadata();
    print("Line: " + ctx.getLine());
    print("File: " + ctx.getFile());
    print("Critical: " + ctx.isCritical());
}

print("Testing throw with Int metadata:");
try {
    int result = divide(10, 0);
} catch (MetadataException<Int> e) {
    print("Caught: " + e.getMessage());
    print("Divisor: " + e.getMetadata());
}

print("Testing throw with String metadata:");
try {
    bool valid = validateInput("");
} catch (MetadataException<String> e) {
    print("Caught: " + e.getMessage());
    print("Error type: " + e.getMetadata());
}

print("Throw generic exception test passed!");
