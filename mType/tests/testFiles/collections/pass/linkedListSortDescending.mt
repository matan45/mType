// Test LinkedList.sortWith() with descending order comparator
import * from "../../lib/collections/LinkedList.mt";
import * from "../../lib/functional/Comparator.mt";
import * from "../../lib/primitives/Int.mt";

// Descending order comparator for Int wrapper objects
class IntDescendingComparator implements Comparator<Int> {
    public function compare(Int a, Int b): int {
        return b.getValue() - a.getValue(); // Reversed comparison
    }

    public function reversed(): Comparator<Int> {
        throw "reversed() not implemented";
    }

    public function thenComparing(Comparator<Int> other): Comparator<Int> {
        throw "thenComparing() not implemented";
    }
}

function main(): void {
    print("Testing LinkedList sortWith descending:");

    LinkedList<Int> list = new LinkedList<Int>();
    list.add(new Int(4));
    list.add(new Int(2));
    list.add(new Int(8));
    list.add(new Int(6));
    list.add(new Int(1));
    list.add(new Int(7));

    print("Before sort:");
    for (Int num : list) {
        print(num);
    }

    // Sort in descending order
    IntDescendingComparator comparator = new IntDescendingComparator();
    list.sortWith(comparator);

    print("After descending sort:");
    for (Int num : list) {
        print(num);
    }

    print("LinkedList descending sort test passed!");
}

main();
