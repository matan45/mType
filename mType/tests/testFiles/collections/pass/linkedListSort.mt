// Test LinkedList.sortWith() functionality
import * from "../../lib/collections/LinkedList.mt";
import * from "../../lib/functional/Comparator.mt";
import * from "../../lib/primitives/Int.mt";

// Simple comparator for Int wrapper objects
class IntComparator implements Comparator<Int> {
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
    print("Testing LinkedList sortWith:");

    LinkedList<Int> list = new LinkedList<Int>();
    list.add(new Int(7));
    list.add(new Int(3));
    list.add(new Int(9));
    list.add(new Int(1));
    list.add(new Int(5));

    print("Before sort:");
    for (Int num : list) {
        print(num);
    }

    // Sort the list
    IntComparator comparator = new IntComparator();
    list.sortWith(comparator);

    print("After sort:");
    for (Int num : list) {
        print(num);
    }

    print("LinkedList sort test passed!");
}

main();
