// Test HashSet as stream source - filter then sort for deterministic order
import * from "../../lib/collections/HashSet.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/functional/Comparator.mt";
import * from "../../lib/primitives/Int.mt";

class IntAsc implements Comparator<Int> {
    public function compare(Int a, Int b): int {
        return a.getValue() - b.getValue();
    }
}

function main(): void {
    print("Testing Stream from HashSet:");

    HashSet<Int> set = new HashSet<Int>();
    set.add(1);
    set.add(2);
    set.add(3);
    set.add(4);
    set.add(5);
    set.add(6);

    // filter even, sort ascending so output is deterministic
    Int[] evens = set.stream()
        .filter(v -> v.getValue() % 2 == 0)
        .sortedWith(new IntAsc())
        .toArray();

    print("Even count: " + evens.length);
    for (Int v : evens) {
        print(v);
    }

    // count via stream (set ordering doesn't matter for count)
    int total = set.stream().count();
    print("Total: " + total);

    print("Stream HashSet source test passed!");
}

main();
