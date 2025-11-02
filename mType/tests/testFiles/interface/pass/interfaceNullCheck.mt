// Test null check before method call
// @Script

interface Logger {
    func log(message: String): void;
}

class ConsoleLogger implements Logger {
    func log(message: String): void {
        print("[LOG] " + message);
    }
}

class Application {
    var logger: Logger;

    func init() {
        this.logger = null;
    }

    func setLogger(logger: Logger): void {
        this.logger = logger;
    }

    func run(): void {
        // Proper null check before calling method
        if (this.logger != null) {
            this.logger.log("Application started");
        } else {
            print("No logger configured");
        }

        // Do some work
        print("Working...");

        if (this.logger != null) {
            this.logger.log("Application finished");
        }
    }
}

var app = new Application();

// Run without logger
app.run();

// Set logger and run again
app.setLogger(new ConsoleLogger());
app.run();
