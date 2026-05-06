// Edge: a lambda is constructed and invoked inside the for-each body, with the
// captured loop value copied into a fresh local (per MYT-215, capturing the
// loop variable directly is rejected).
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

interface IntFn {
    function apply(int x): int;
}

function main(): void {
    ArrayList<Int> a = new ArrayList<Int>();
    a.add(new Int(5));
    a.add(new Int(7));

    for (Int x : a) {
        int captured = x.getValue();
        IntFn doubler = n -> n * captured;
        print(doubler.apply(2));
    }
}

main();
