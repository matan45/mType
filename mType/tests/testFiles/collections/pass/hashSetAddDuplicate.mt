// Test: HashSet.add() of a duplicate returns false and does not grow size
import * from "../../lib/collections/HashSet.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    HashSet<Int> s = new HashSet<Int>();
    bool a = s.add(new Int(5));
    bool b = s.add(new Int(5));
    print("first add: " + a);
    print("dup add: " + b);
    print("size: " + s.size());
    print("contains 5: " + s.contains(new Int(5)));
}
main();

// Expected output:
// first add: true
// dup add: false
// size: 1
// contains 5: true
