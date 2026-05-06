// Edge: iterating directly over the return value of a method call. The
// returned collection lives only as long as the for-each setup; the iterator
// must keep it alive across all iterations.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function makeList(): ArrayList<Int> {
    ArrayList<Int> r = new ArrayList<Int>();
    r.add(new Int(7));
    r.add(new Int(8));
    r.add(new Int(9));
    return r;
}

function main(): void {
    for (Int v : makeList()) {
        print(v.getValue());
    }
}

main();
