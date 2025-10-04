// Test invalid combinations and edge cases
class EdgeCaseHandler {
    final int MAX_SAFE_VALUE = 1000;

    public function handleEdgeCases(int value, string operation): string {
        if (value < 0) {
            return "ERROR: Negative value not supported";
        }
        
        if (value > MAX_SAFE_VALUE) {
            return "ERROR: Value exceeds maximum safe limit";
        }
        
        if (operation == "divide" && value == 0) {
            return "ERROR: Division by zero";
        }
        
        if (operation == "unknown") {
            return "ERROR: Unknown operation";
        }
        
        return "OK: Valid combination";
    }
}

// Test various invalid combinations
EdgeCaseHandler handler = new EdgeCaseHandler();

print(handler.handleEdgeCases(-5, "add"));
print(handler.handleEdgeCases(1500, "multiply"));
print(handler.handleEdgeCases(0, "divide"));
print(handler.handleEdgeCases(100, "unknown"));
print(handler.handleEdgeCases(50, "add"));