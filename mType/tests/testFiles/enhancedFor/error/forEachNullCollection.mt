// Edge: for-each over a nullable collection that is actually null. Either
// the type checker rejects iterating a `T?` directly, or the runtime must
// throw on GET_ITERATOR — never segfault.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayList<Int>? maybe = null;
    for (Int x : maybe) {
        print(x.getValue());
    }
}

main();
