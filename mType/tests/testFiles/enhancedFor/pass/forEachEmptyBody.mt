// Edge: empty body. The iterator setup, hasNext/next sequence, and cleanup
// must still execute even when the body emits nothing.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayList<Int> a = new ArrayList<Int>();
    a.add(new Int(1));
    a.add(new Int(2));
    a.add(new Int(3));

    for (Int x : a) {
    }
    print("done");
}

main();
