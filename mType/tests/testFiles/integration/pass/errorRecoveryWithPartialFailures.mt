// Test error recovery scenarios
class FailureProneOperation {
    int attempt;
    string operation;
    
    constructor(string op) {
        operation = op;
        attempt = 0;
    }
    
    function tryOperation(): string {
        attempt = attempt + 1;
        
        if (attempt <= 2) {
            return "RETRY: " + operation + " attempt " + toString(attempt);
        } else if (attempt == 3) {
            return "SUCCESS: " + operation + " succeeded on attempt " + toString(attempt);
        } else {
            return "FAILED: " + operation + " failed after " + toString(attempt) + " attempts";
        }
    }
}

function batchOperationWithRecovery(): string {
    string results = "";
    final int NUM_OPERATIONS = 3;
    
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        FailureProneOperation op = new FailureProneOperation("Op" + toString(i));
        
        // Try operation multiple times
        for (int retry = 0; retry < 4; retry++) {
            string result = op.tryOperation();
            results = results + result + " | ";
            
            if (result != "RETRY: Op" + toString(i) + " attempt " + toString(retry + 1)) {
                break;
            }
        }
    }
    
    return results;
}

// Test error recovery
string recoveryResults = batchOperationWithRecovery();
print(recoveryResults);