// Test: HashSet iterator on an empty set reports hasNext() false immediately
import * from "../../lib/collections/HashSet.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    HashSet<Int> s = new HashSet<Int>();
    Iterator<Int> iter = s.iterator();
    print("hasNext: " + iter.hasNext());
    iter.close();
    print("done");
}
main();

// Expected output:
// hasNext: false
// done
