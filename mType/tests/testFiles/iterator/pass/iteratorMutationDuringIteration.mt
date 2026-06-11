// Test: pins BY-DESIGN behavior — mType iterators do NOT throw a
// concurrent-modification error when the source list is structurally
// modified mid-iteration (matching the documented for-each behavior in
// EnhancedForLoopTestSuite). If this starts failing because an exception
// appears, that is a deliberate language change, not a regression here.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));
    list.add(new Int(3));

    Iterator<Int> iter = list.iterator();
    int seen = 0;
    while (iter.hasNext()) {
        Int v = iter.next();
        seen = seen + 1;
        if (v.getValue() == 2) {
            list.add(new Int(99));
        }
    }
    iter.close();
    print("seen: " + seen);
    print("final size: " + list.size());
}
main();
