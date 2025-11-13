// Test ArrayList.sortWith() functionality
import * from "../../lib/collections/ArrayList.mt";
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
    print("Testing ArrayList sortWith:");

    ArrayList<Int> list = new ArrayList<Int>();
    list.add(5);
    list.add(2);
    list.add(8);
    list.add(1);
    list.add(9);

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

    print("ArrayList sort test passed!");
}
main();
