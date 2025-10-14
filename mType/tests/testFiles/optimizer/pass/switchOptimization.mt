// Test file for switch/case optimization
// Tests that the optimizer correctly handles switch statements

class StatusHandler {
    private string status;

    constructor(string s) {
        this.status = s;
    }

    // Used method - contains switch statement
    public function process(): void {
        switch (this.status) {
            case "active":
                print("Status is active");
                break;
            case "inactive":
                print("Status is inactive");
                break;
            default:
                print("Unknown status");
        }
    }

    // UNUSED method - should be removed
    public function getStatus(): string {
        return this.status;
    }

    // UNUSED method - should be removed
    public function setStatus(string newStatus): void {
        this.status = newStatus;
    }
}

// UNUSED class - should be removed
class UnusedHandler {
    public function handle(int code): string {
        switch (code) {
            case 1:
                return "One";
            case 2:
                return "Two";
            default:
                return "Unknown";
        }
    }
}

// Used function
function testSwitch(): void {
    StatusHandler handler = new StatusHandler("active");
    handler.process();
}

// UNUSED function - should be removed
function unusedSwitch(int value): string {
    switch (value) {
        case 0:
            return "Zero";
        case 1:
            return "One";
        default:
            return "Other";
    }
}

// ===== ENTRY POINT CODE =====

testSwitch();
print("Switch optimization test complete!");
