// Test Stack as stream source - reduce and count
import * from "../../lib/collections/Stack.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing Stream from Stack:");

    Stack<Int> stack = new Stack<Int>();
    stack.push(new Int(5));
    stack.push(new Int(10));
    stack.push(new Int(15));
    stack.push(new Int(20));

    // count is order-insensitive
    int total = stack.stream().count();
    print("Count: " + total);

    // sum via reduceWithIdentity is order-insensitive
    Int sum = stack.stream().reduceWithIdentity(new Int(0), (a, b) -> a.add(b));
    print("Sum: " + sum.getValue());

    // anyMatch / allMatch don't depend on order
    bool hasGT12 = stack.stream().anyMatch(v -> v.getValue() > 12);
    bool allPositive = stack.stream().allMatch(v -> v.getValue() > 0);
    print("Has >12: " + hasGT12);
    print("All positive: " + allPositive);

    print("Stream Stack source test passed!");
}

main();
