// Test async/await in loop iterations
// Validates async operations within different loop constructs

class LoopItem {
    int index;
    int value;

    public constructor(int idx, int val) {
        this.index = idx;
        this.value = val;
    }

    public function getIndex(): int {
        return this.index;
    }

    public function getValue(): int {
        return this.value;
    }
}

print("=== Async in Loop Test ===");

// Async function to process single item
function async processItem(int i): Promise<LoopItem> {
    print("Processing item " + i);
    int computed = i * i;
    LoopItem item = new LoopItem(i, computed);
    return item;
}

// Test 1: Await in for loop
function async testForLoop(): Promise<void> {
    print("");
    print("Test 1: Await in for loop");
    int total = 0;

    for (int i = 1; i <= 5; i = i + 1) {
        LoopItem item = await processItem(i);
        print("Got item " + item.getIndex() + " with value " + item.getValue());
        total = total + item.getValue();
    }

    print("Total sum: " + total);
    print("Expected: 1 + 4 + 9 + 16 + 25 = 55");
}

// Test 2: Await in while loop
function async testWhileLoop(): Promise<void> {
    print("");
    print("Test 2: Await in while loop");
    int counter = 1;
    int sum = 0;

    while (counter <= 3) {
        LoopItem item = await processItem(counter);
        print("While loop item " + item.getIndex() + ": " + item.getValue());
        sum = sum + item.getValue();
        counter = counter + 1;
    }

    print("While loop sum: " + sum);
}

// Test 3: Conditional await in loop
function async testConditionalInLoop(): Promise<void> {
    print("");
    print("Test 3: Conditional await in loop");
    int evenSum = 0;
    int oddSum = 0;

    for (int i = 1; i <= 6; i = i + 1) {
        LoopItem item = await processItem(i);

        if (i % 2 == 0) {
            print("Even item " + i + ": " + item.getValue());
            evenSum = evenSum + item.getValue();
        } else {
            print("Odd item " + i + ": " + item.getValue());
            oddSum = oddSum + item.getValue();
        }
    }

    print("Even sum: " + evenSum);
    print("Odd sum: " + oddSum);
}

// Test 4: Break with await in loop
function async testBreakInLoop(): Promise<void> {
    print("");
    print("Test 4: Break with await in loop");

    for (int i = 1; i <= 10; i = i + 1) {
        LoopItem item = await processItem(i);
        print("Checking item " + item.getIndex());

        if (item.getValue() >= 25) {
            print("Found value >= 25, breaking at index " + item.getIndex());
            break;
        }
    }
}

// Test 5: Continue with await in loop
function async testContinueInLoop(): Promise<void> {
    print("");
    print("Test 5: Continue with await in loop");
    int processedCount = 0;

    for (int i = 1; i <= 7; i = i + 1) {
        LoopItem item = await processItem(i);

        // Skip items with odd indices
        if (item.getIndex() % 2 == 1) {
            print("Skipping odd index " + item.getIndex());
            continue;
        }

        print("Processing even index " + item.getIndex() + " with value " + item.getValue());
        processedCount = processedCount + 1;
    }

    print("Processed " + processedCount + " even-indexed items");
}

// Test 6: Nested loops with await
function async testNestedLoops(): Promise<void> {
    print("");
    print("Test 6: Nested loops with await");

    for (int i = 1; i <= 2; i = i + 1) {
        print("Outer loop iteration " + i);

        for (int j = 1; j <= 3; j = j + 1) {
            int combined = i * 10 + j;
            LoopItem item = await processItem(combined);
            print("  Inner [" + i + "," + j + "]: " + item.getValue());
        }
    }
}

// Main function
function async main(): Promise<void> {
    testForLoop();
    testWhileLoop();
    testConditionalInLoop();
    testBreakInLoop();
    testContinueInLoop();
    testNestedLoops();

    print("");
    print("All async in loop tests complete");
}

main();
