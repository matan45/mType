// Test: Method override with parameter type change (invariant violation)
// Expected: Error - method parameters must match exactly in override

class DataProcessor {
    public function process(string data): string {
        return "Processing: " + data;
    }

    public function validate(int count): bool {
        return count > 0;
    }
}

class AdvancedProcessor extends DataProcessor {
    // ERROR: Parameter type changed from string to int
    // Method parameters are invariant - they cannot change type in override
    public function process(int data): string {
        return "Advanced processing: " + data;
    }

    // This would also be an error if uncommented
    // ERROR: Parameter type changed from int to float
    public function validate(float count): bool {
        return count > 0.0;
    }
}

// This should fail at type checking
AdvancedProcessor proc = new AdvancedProcessor();
print(proc.process(42));
