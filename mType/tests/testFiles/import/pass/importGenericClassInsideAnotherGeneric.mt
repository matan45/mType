// Edge: nested generics across imports. ArrayList<Int> from one library
// embedded inside Box<T> from another — exercises type substitution across
// module boundaries.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Box.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayList<Int> inner = new ArrayList<Int>();
    inner.add(new Int(5));

    Box<ArrayList<Int>> b = new Box<ArrayList<Int>>(inner);
    // Extract via typed local — for-each on `b.getValue()` directly loses the
    // ArrayList<Int> substitution and falls back to Object (separate issue).
    ArrayList<Int> unwrapped = b.getValue();
    for (Int v : unwrapped) {
        print(v.getValue());
    }
}

main();
