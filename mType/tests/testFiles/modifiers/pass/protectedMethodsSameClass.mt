// Test: Protected methods accessible within same class
class Logger {
    protected function logInternal(string message): void {
        print("[LOG] " + message);
    }

    public function log(string message): void {
        // Calling protected method from same class
        logInternal(message);
    }
}

Logger logger = new Logger();
logger.log("Test message");  // Expected: [LOG] Test message
