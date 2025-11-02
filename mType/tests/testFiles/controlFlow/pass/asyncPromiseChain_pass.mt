// Test promise chains and sequential async operations
// Validates sequential async calls with data dependencies

class Pipeline {
    string stageName;
    int stageValue;

    public constructor(string name, int value) {
        this.stageName = name;
        this.stageValue = value;
    }

    public function getName(): string {
        return this.stageName;
    }

    public function getValue(): int {
        return this.stageValue;
    }
}

print("=== Promise Chain Test ===");

// Stage 1: Input validation
function async validateInput(int input): Promise<Pipeline> {
    print("Stage 1: Validating input " + input);
    Pipeline stage = new Pipeline("Validation", input);
    return stage;
}

// Stage 2: Transformation
function async transformData(Pipeline prev): Promise<Pipeline> {
    print("Stage 2: Transforming data from " + prev.getName());
    int transformed = prev.getValue() * 3;
    Pipeline stage = new Pipeline("Transformation", transformed);
    return stage;
}

// Stage 3: Processing
function async processData(Pipeline prev): Promise<Pipeline> {
    print("Stage 3: Processing data from " + prev.getName());
    int processed = prev.getValue() + 100;
    Pipeline stage = new Pipeline("Processing", processed);
    return stage;
}

// Stage 4: Finalization
function async finalizeData(Pipeline prev): Promise<Pipeline> {
    print("Stage 4: Finalizing data from " + prev.getName());
    int finalized = prev.getValue() - 50;
    Pipeline stage = new Pipeline("Finalization", finalized);
    return stage;
}

// Test sequential chaining
function async runPipeline(int input): Promise<Pipeline> {
    print("Starting pipeline with input: " + input);

    // Sequential async operations with dependencies
    Pipeline stage1 = await validateInput(input);
    print("Completed " + stage1.getName() + " with value " + stage1.getValue());

    Pipeline stage2 = await transformData(stage1);
    print("Completed " + stage2.getName() + " with value " + stage2.getValue());

    Pipeline stage3 = await processData(stage2);
    print("Completed " + stage3.getName() + " with value " + stage3.getValue());

    Pipeline stage4 = await finalizeData(stage3);
    print("Completed " + stage4.getName() + " with value " + stage4.getValue());

    return stage4;
}

// Main function
function async main(): Promise<void> {
    Pipeline result = await runPipeline(10);
    print("Final pipeline result: " + result.getValue());
    print("Expected: ((10 * 3) + 100) - 50 = 80");

    print("Promise chain complete");
    return null;
}

main();
