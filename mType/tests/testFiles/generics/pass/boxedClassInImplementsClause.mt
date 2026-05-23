// MYT-360 regression guard: the boxed-class form of the ticket repro must
// still compile and run after the parser now rejects `Predicate<int>`.

import * from "../../lib/primitives/Int.mt";

interface Predicate<T> {
    function test(T value): bool;
}

class PositivePredicate implements Predicate<Int> {
    public function test(Int value): bool {
        return value.getValue() > 0;
    }
}

function main(): void {
    PositivePredicate p = new PositivePredicate();
    if (p.test(new Int(5))) {
        print("positive");
    } else {
        print("non-positive");
    }
    if (p.test(new Int(-3))) {
        print("positive");
    } else {
        print("non-positive");
    }
}

main();
