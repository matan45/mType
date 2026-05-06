// Test that hasNext() can be called repeatedly on an empty iterator without throwing
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing repeated hasNext on empty:");

    ArrayList<Int> list = new ArrayList<Int>();
    Iterator<Int> iter = list.iterator();

    print("call 1: " + iter.hasNext());
    print("call 2: " + iter.hasNext());
    print("call 3: " + iter.hasNext());

    iter.close();

    print("repeated hasNext empty test passed!");
}
main();
