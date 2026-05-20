// Test: Stack.pop() and Stack.peek() on an empty stack return null
import * from "../../lib/collections/Stack.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    Stack<Int> s = new Stack<Int>();
    print("empty: " + s.empty());
    print("size: " + s.size());

    Int p = s.pop();
    print("pop empty null: " + (p == null));

    Int pk = s.peek();
    print("peek empty null: " + (pk == null));

    s.push(new Int(7));
    Int p2 = s.pop();
    print("pop after push: " + p2.getValue());

    Int p3 = s.pop();
    print("pop empty again null: " + (p3 == null));
}
main();

// Expected output:
// empty: true
// size: 0
// pop empty null: true
// peek empty null: true
// pop after push: 7
// pop empty again null: true
