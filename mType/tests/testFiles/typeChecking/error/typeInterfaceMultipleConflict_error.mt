// Test conflicting default methods error - multiple interfaces with same method signature but different implementations
interface Logger {
    function log(string message): void;
    function getLevel(): string;
}

interface Printer {
    function print(string message): void;
    function getLevel(): string;
}

// Error: Both Logger and Printer define getLevel()
// If they have conflicting implementations, this should error
class ConflictingImpl implements Logger, Printer {
    public function log(string message): void {
        print("Log: " + message);
    }

    public function print(string message): void {
        print("Print: " + message);
    }

    // Error: Ambiguous which getLevel() to implement
    public function getLevel(): string {
        return "INFO";
    }
}

ConflictingImpl impl = new ConflictingImpl();
impl.log("test");
print("This should fail");
