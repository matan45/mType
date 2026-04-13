// Test null check before method call
// @Script

interface Logger {
    function log(string message): void;
}

class ConsoleLogger implements Logger {
    public function log(string message): void {
        print("[LOG] " + message);
    }
}

class Application {
    private Logger? logger;

    public function init() {
        this.logger = null;
    }

    public function setLogger(Logger logger): void {
        this.logger = logger;
    }

    public function run(): void {
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

Application app = new Application();

// Run without logger
app.run();

// Set logger and run again
app.setLogger(new ConsoleLogger());
app.run();
