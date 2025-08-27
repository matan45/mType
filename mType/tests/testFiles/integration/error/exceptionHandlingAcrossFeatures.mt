// Test error conditions and edge cases
namespace errorProne {
    final int DANGER_THRESHOLD = 50;
    
    class RiskyOperation {
        int riskLevel;
        string operation;
        
        constructor(string op, int risk) {
            operation = op;
            riskLevel = risk;
        }
        
        function executeOperation(): string {
            if (riskLevel > DANGER_THRESHOLD) {
                return "HIGH_RISK: " + operation + " risk level " + toString(riskLevel);
            } else if (riskLevel > 20) {
                return "MEDIUM_RISK: " + operation + " risk level " + toString(riskLevel);
            } else {
                return "LOW_RISK: " + operation + " risk level " + toString(riskLevel);
            }
        }
        
        function isHighRisk(): bool {
            return riskLevel > DANGER_THRESHOLD;
        }
    }
    
    function batchRiskyOperations(): string {
        string results = "";
        final int NUM_OPERATIONS = 5;
        int operationRisks[5] = {10, 25, 60, 15, 80};
        
        for (int i = 0; i < NUM_OPERATIONS; i++) {
            RiskyOperation op = new RiskyOperation("Operation" + toString(i), operationRisks[i]);
            
            string result = op.executeOperation();
            results = results + result + " | ";
            
            if (op.isHighRisk()) {
                results = results + "ESCALATED | ";
            }
        }
        
        return results;
    }
}

// Test exception-prone scenarios
string batchResults = errorProne::batchRiskyOperations();
print(batchResults);