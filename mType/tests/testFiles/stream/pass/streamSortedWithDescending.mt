// Test Stream.sortedWith() with descending comparator
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/functional/Comparator.mt";
import * from "../../lib/primitives/Int.mt";

// Descending order comparator for Int wrapper objects
class IntDescendingComparator implements Comparator<Int> {
    public function compare(Int a, Int b): int {
        return b.getValue() - a.getValue(); // Reversed
    }
}

function main(): void {
    print("Testing Stream sortedWith descending:");

    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(3));
    list.add(new Int(7));
    list.add(new Int(1));
    list.add(new Int(9));
    list.add(new Int(4));
    list.add(new Int(6));

    print("Original order:");
    for (Int num : list) {
        print(num);
    }

    // Sort in descending order using stream
    IntDescendingComparator comparator = new IntDescendingComparator();
    Stream<Int> stream = list.stream().sortedWith(comparator);
    Int[] sorted = stream.toArray();

    print("After sortedWith descending:");
    for (Int num : sorted) {
        print(num);
    }

    print("Stream sortedWith descending test passed!");
}

main();
