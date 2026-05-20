// Test: Stack iterator yields elements from top to bottom
import * from "../../lib/collections/Stack.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    Stack<Int> s = new Stack<Int>();
    s.push(new Int(1));
    s.push(new Int(2));
    s.push(new Int(3));

    Iterator<Int> iter = s.iterator();
    while (iter.hasNext()) {
        Int v = iter.next();
        print(v.getValue());
    }
    iter.close();
    print("done");
}
main();

// Expected output:
// 3
// 2
// 1
// done
