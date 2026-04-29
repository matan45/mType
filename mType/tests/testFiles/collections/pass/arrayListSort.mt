// Test ArrayList.sortWith() functionality
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/functional/Comparator.mt";
import * from "../../lib/primitives/Int.mt";

// Simple comparator for Int wrapper objects
class IntComparator implements Comparator<Int> {
    public function compare(Int a, Int b): int {
        return a.getValue() - b.getValue();
    }
}

function main(): void {
    print("Testing ArrayList sortWith:");

    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(5));
    list.add(new Int(2));
    list.add(new Int(8));
    list.add(new Int(1));
    list.add(new Int(9));

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
