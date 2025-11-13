// Test enhanced for-loop with Stack
import * from "../../lib/collections/Stack.mt";

@Script
function main(): void {
    // Create a stack
    Stack<int> stack = new Stack<int>();
    stack.push(100);
    stack.push(200);
    stack.push(300);

    // Test enhanced for-loop (iterates top to bottom)
    print("Testing enhanced for-loop with Stack:");
    for (int value : stack) {
        print(value);
    }

    print("Enhanced for-loop with Stack test passed!");
}
