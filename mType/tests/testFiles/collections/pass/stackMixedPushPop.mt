// Test Stack LIFO ordering after interleaved push/pop sequence
import * from "../../lib/collections/Stack.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing Stack mixed push/pop:");

    Stack<Int> s = new Stack<Int>();
    s.push(new Int(1));
    s.push(new Int(2));
    s.push(new Int(3));
    s.push(new Int(4));
    s.push(new Int(5));

    Int p1 = s.pop();
    print("pop: " + p1.getValue());
    Int p2 = s.pop();
    print("pop: " + p2.getValue());

    s.push(new Int(6));
    s.push(new Int(7));

    while (!s.empty()) {
        Int v = s.pop();
        print("pop: " + v.getValue());
    }

    print("Stack mixed push/pop test passed!");
}
main();
