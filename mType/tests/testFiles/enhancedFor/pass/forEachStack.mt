// Test enhanced for-loop with Stack
import * from "../../lib/collections/Stack.mt";
import * from "../../lib/primitives/Int.mt";

@Script
function main(): void {
    // Create a stack
    Stack<Int> stack = new Stack<Int>();
    stack.push(100);
    stack.push(200);
    stack.push(300);

    // Test enhanced for-loop (iterates top to bottom)
    print("Testing enhanced for-loop with Stack:");
    for (Int valueObj : stack) {
        int value = valueObj.getValue();
        print(value);
    }

    print("Enhanced for-loop with Stack test passed!");
}
