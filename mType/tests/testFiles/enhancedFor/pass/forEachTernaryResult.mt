// CANARY (MYT-350): for-each over a ternary expression loses the iterable's
// element type — same family as the nested-generic canaries. `pickA ? a : b`
// (both ArrayList<Int>) collapses to Object at the for-each position, so the
// for-each header rejects with MT-E2007. This widens the observed surface
// beyond nested collections: any Object-typed iterable expression hits the
// same inference gap. See memory:project_foreach_loses_nested_generic_type.
// Keep failing until fixed.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayList<Int> a = new ArrayList<Int>();
    a.add(new Int(1));
    a.add(new Int(2));

    ArrayList<Int> b = new ArrayList<Int>();
    b.add(new Int(10));
    b.add(new Int(20));
    b.add(new Int(30));

    bool pickA = false;
    int total = 0;
    for (Int v : pickA ? a : b) {
        total = total + v.getValue();
    }
    print("total=" + total);
}

main();

// Expected output:
// total=60
