// Edge: reassigning the loop variable inside the body must not affect the
// next iteration. The for-each pulls a fresh value from the iterator each
// time and the local-variable write is purely local-scope.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));
    list.add(new Int(3));

    for (Int v : list) {
        int observed = v.getValue();
        v = new Int(999);
        print("observed=" + observed + " local=" + v.getValue());
    }
}

main();

// Expected output:
// observed=1 local=999
// observed=2 local=999
// observed=3 local=999
