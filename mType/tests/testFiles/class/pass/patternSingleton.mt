// Test: Complete singleton pattern implementation
// Expected: Pass - demonstrates singleton design pattern

class Logger {
    private static Logger instance;
    private int messageCount;
    private string logLevel;

    // Private constructor prevents external instantiation
    private constructor() {
        this.messageCount = 0;
        this.logLevel = "INFO";
        print("[Logger] Singleton instance created");
    }

    // Public static method to get the singleton instance
    public static function getInstance(): Logger {
        if (instance == null) {
            instance = new Logger();
        }
        return instance;
    }

    public function setLogLevel(string level): void {
        this.logLevel = level;
        print("[Logger] Log level set to: " + level);
    }

    public function log(string message): void {
        this.messageCount = this.messageCount + 1;
        print("[" + this.logLevel + "] Message #" + this.messageCount + ": " + message);
    }

    public function getMessageCount(): int {
        return this.messageCount;
    }

    public function reset(): void {
        this.messageCount = 0;
        print("[Logger] Message count reset");
    }
}

// Test singleton pattern
print("Test 1: Get first instance");
Logger logger1 = Logger::getInstance();
logger1.log("First message");

print("\nTest 2: Get second instance");
Logger logger2 = Logger::getInstance();
logger2.log("Second message");

print("\nTest 3: Verify same instance");
print("Same instance: " + (logger1 == logger2));
print("Message count from logger1: " + logger1.getMessageCount());
print("Message count from logger2: " + logger2.getMessageCount());

print("\nTest 4: Modify through one reference");
logger1.setLogLevel("DEBUG");
logger2.log("Third message");

print("\nTest 5: Reset and continue");
logger1.reset();
logger2.log("Fourth message");
print("Final count: " + logger1.getMessageCount());
