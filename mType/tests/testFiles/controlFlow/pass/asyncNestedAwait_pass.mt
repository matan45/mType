// Test nested async function calls with multiple await levels
// Validates deep async call chains and control flow through layers

class Task {
    int level;
    int accumulator;

    public constructor(int lvl, int acc) {
        this.level = lvl;
        this.accumulator = acc;
    }

    public function getLevel(): int {
        return this.level;
    }

    public function getAccumulator(): int {
        return this.accumulator;
    }

    public function addValue(int value): void {
        this.accumulator = this.accumulator + value;
    }
}

print("=== Nested Async Await Test ===");

// Level 1: Base async function
function async createTask(): Promise<Task> {
    print("Level 1: Creating task");
    Task t = new Task(1, 10);
    return t;
}

// Level 2: Awaits level 1
function async processTask(): Promise<Task> {
    print("Level 2: Processing task");
    Task t = await createTask();
    print("Level 2: Got task with accumulator " + t.getAccumulator());
    t.addValue(20);
    return t;
}

// Level 3: Awaits level 2
function async enhanceTask(): Promise<Task> {
    print("Level 3: Enhancing task");
    Task t = await processTask();
    print("Level 3: Got task with accumulator " + t.getAccumulator());
    t.addValue(30);
    return t;
}

// Level 4: Awaits level 3
function async finalizeTask(): Promise<Task> {
    print("Level 4: Finalizing task");
    Task t = await enhanceTask();
    print("Level 4: Got task with accumulator " + t.getAccumulator());
    t.addValue(40);
    return t;
}

// Main function: Awaits level 4
function async main(): Promise<void> {
    print("Starting nested async operations");

    Task finalTask = await finalizeTask();
    print("Final accumulator value: " + finalTask.getAccumulator());
    print("Expected: 10 + 20 + 30 + 40 = 100");

    print("Nested async operations complete");
}

main();
