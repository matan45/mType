// Test: A `static final` field with a generic-collection initializer is
// initialized once at class load. Multiple reads return the same instance —
// observed via shared mutation visible across reads, plus a count check.

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

class Holder {
    public static final ArrayList<Int> CONST = new ArrayList<Int>();

    public static function add(int v): void {
        Holder::CONST.add(new Int(v));
    }

    public static function size(): int {
        return Holder::CONST.size();
    }
}

function main(): void {
    print("initial size: " + Holder::size());

    Holder::add(1);
    Holder::add(2);
    Holder::add(3);

    // Re-read the same static through different code paths — same instance.
    ArrayList<Int> a = Holder::CONST;
    ArrayList<Int> b = Holder::CONST;

    print("a size: " + a.size());
    print("b size: " + b.size());
    print("Holder::size(): " + Holder::size());

    // Shared identity: a mutation through `a` is visible through `b`.
    a.add(new Int(99));
    print("after a.add, b size: " + b.size());
    print("after a.add, b last: " + b.get(b.size() - 1).getValue());
}

main();
