// Edge: two consecutive for-each loops over the same collection. The
// synthetic iterator slot must be cleanly scoped — if it leaks, the second
// loop sees stale state from the first.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayList<Int> a = new ArrayList<Int>();
    a.add(new Int(1));
    a.add(new Int(2));

    int sum = 0;
    for (Int x : a) {
        sum = sum + x.getValue();
    }
    for (Int x : a) {
        sum = sum + x.getValue();
    }
    print(sum);
}

main();
