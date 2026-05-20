// Test: Stack.peek() reflects the current top across interleaved push/pop
import * from "../../lib/collections/Stack.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    Stack<Int> s = new Stack<Int>();
    s.push(new Int(1));
    print("peek after push 1: " + s.peek().getValue());

    s.push(new Int(2));
    print("peek after push 2: " + s.peek().getValue());

    s.push(new Int(3));
    print("peek after push 3: " + s.peek().getValue());

    s.pop();
    print("peek after pop: " + s.peek().getValue());

    s.push(new Int(99));
    print("peek after push 99: " + s.peek().getValue());

    s.pop();
    s.pop();
    print("peek after two pops: " + s.peek().getValue());
}
main();

// Expected output:
// peek after push 1: 1
// peek after push 2: 2
// peek after push 3: 3
// peek after pop: 2
// peek after push 99: 99
// peek after two pops: 1
