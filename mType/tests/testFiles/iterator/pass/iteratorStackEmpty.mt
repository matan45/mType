// Test: Stack iterator on an empty stack reports hasNext() false immediately
import * from "../../lib/collections/Stack.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    Stack<Int> s = new Stack<Int>();
    Iterator<Int> iter = s.iterator();
    print("hasNext: " + iter.hasNext());
    iter.close();
    print("done");
}
main();

// Expected output:
// hasNext: false
// done
