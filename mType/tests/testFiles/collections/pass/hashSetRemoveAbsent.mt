// Test: HashSet.remove() of a non-existent element returns false, size unchanged
import * from "../../lib/collections/HashSet.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    HashSet<Int> s = new HashSet<Int>();
    s.add(new Int(1));
    s.add(new Int(2));
    s.add(new Int(3));

    bool r = s.remove(new Int(999));
    print("remove absent: " + r);
    print("size: " + s.size());
    print("still has 1: " + s.contains(new Int(1)));

    bool r2 = s.remove(new Int(2));
    print("remove present: " + r2);
    print("size after: " + s.size());
}
main();

// Expected output:
// remove absent: false
// size: 3
// still has 1: true
// remove present: true
// size after: 2
