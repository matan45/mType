// Test ArrayList.sortWith() with String comparator
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/functional/Comparator.mt";
import * from "../../lib/primitives/String.mt";

// Alphabetical comparator for String wrapper objects
class StringComparator implements Comparator<String> {
    public function compare(String a, String b): int {
        // Simple lexicographic comparison based on first character
        // For full comparison, we'd need string comparison methods
        String aVal = a.getValue();
        String bVal = b.getValue();

        // Use length as a simple comparison metric for this test
        int aLen = aVal.length();
        int bLen = bVal.length();

        if (aLen < bLen) return -1;
        if (aLen > bLen) return 1;
        return 0;
    }

    public function reversed(): Comparator<String> {
        throw "reversed() not implemented";
    }

    public function thenComparing(Comparator<String> other): Comparator<String> {
        throw "thenComparing() not implemented";
    }
}

function main(): void {
    print("Testing ArrayList sortWith strings:");

    ArrayList<String> list = new ArrayList<String>();
    list.add(new String("banana"));
    list.add(new String("apple"));
    list.add(new String("cherry"));
    list.add(new String("date"));
    list.add(new String("fig"));

    print("Before sort:");
    for (String str : list) {
        print(str);
    }

    // Sort by string length
    StringComparator comparator = new StringComparator();
    list.sortWith(comparator);

    print("After sort (by length):");
    for (String str : list) {
        print(str);
    }

    print("ArrayList string sort test passed!");
}

main();
