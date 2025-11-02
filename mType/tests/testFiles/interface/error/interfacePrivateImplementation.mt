// Test private implementation of interface method (should error)
// @Throw

interface Logger {
    func log(message: String): void;
}

class FileLogger implements Logger {
    var filename: String;

    func init(filename: String) {
        this.filename = filename;
    }

    // Error: Cannot implement interface method as private
    private func log(message: String): void {
        print("[" + this.filename + "] " + message);
    }
}

var logger = new FileLogger("app.log");
logger.log("Test message");  // Should fail - method is private
