// Test: Protected methods accessible within same class
class Logger {
    protected void logInternal(string message) {
        print("[LOG] " + message);
    }

    public void log(string message) {
        // Calling protected method from same class
        logInternal(message);
    }
}

Logger logger = new Logger();
logger.log("Test message");  // Expected: [LOG] Test message
