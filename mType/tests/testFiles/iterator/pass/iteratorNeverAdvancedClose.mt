// Test that close() works on an iterator that was never advanced, and a fresh
// iterator on the same collection still iterates from the start.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing close without advance:");

    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));
    list.add(new Int(3));

    Iterator<Int> iter = list.iterator();
    iter.close();

    Iterator<Int> iter2 = list.iterator();
    while (iter2.hasNext()) {
        Int v = iter2.next();
        print(v);
    }
    iter2.close();

    print("close without advance test passed!");
}
main();
