// Error: structural mutation of the very collection being iterated. Adding
// to `list` from inside its own for-each should fail at runtime
// (ConcurrentModificationException-style). Exact error string captured on
// first build, then pinned with expectedErrorSubstring.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));
    list.add(new Int(3));

    for (Int v : list) {
        list.add(new Int(v.getValue() + 100));
    }
    print("should not reach here");
}

main();
