// Test: calling close() twice on an iterator is a no-op (does not throw)
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(1));
    Iterator<Int> iter = list.iterator();
    Int v = iter.next();
    print("first: " + v.getValue());
    iter.close();
    iter.close();
    print("closed twice ok");
}
main();

// Expected output:
// first: 1
// closed twice ok
