// Test Stream.sortedWith() with custom comparator
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/functional/Comparator.mt";
import * from "../../lib/primitives/Int.mt";

// Ascending order comparator for Int wrapper objects
class IntAscendingComparator implements Comparator<Int> {
    public function compare(Int a, Int b): int {
        return a.getValue() - b.getValue();
    }

    public function reversed(): Comparator<Int> {
        throw "reversed() not implemented";
    }

    public function thenComparing(Comparator<Int> other): Comparator<Int> {
        throw "thenComparing() not implemented";
    }
}

function main(): void {
    print("Testing Stream sortedWith:");

    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(5));
    list.add(new Int(2));
    list.add(new Int(9));
    list.add(new Int(1));
    list.add(new Int(7));

    print("Original order:");
    for (Int num : list) {
        print(num);
    }

    // Sort using stream
    IntAscendingComparator comparator = new IntAscendingComparator();
    Stream<Int> stream = list.stream().sortedWith(comparator);
    Int[] sorted = stream.toArray();

    print("After sortedWith:");
    for (Int num : sorted) {
        print(num);
    }

    print("Stream sortedWith test passed!");
}

main();
