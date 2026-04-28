// Test ArrayList.sortWith() with descending order comparator
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/functional/Comparator.mt";
import * from "../../lib/primitives/Int.mt";

// Descending order comparator for Int wrapper objects
class IntDescendingComparator implements Comparator<Int> {
    public function compare(Int a, Int b): int {
        return b.getValue() - a.getValue(); // Reversed comparison
    }
}

function main(): void {
    print("Testing ArrayList sortWith descending:");

    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(5));
    list.add(new Int(2));
    list.add(new Int(8));
    list.add(new Int(1));
    list.add(new Int(9));
    list.add(new Int(3));

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

    print("ArrayList descending sort test passed!");
}

main();
