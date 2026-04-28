// Test Stream.sortedWith() chained with filter and map operations
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/functional/Comparator.mt";
import * from "../../lib/functional/Function.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/exceptions/RuntimeException.mt";

// Comparator for Int wrapper objects
class IntComparator implements Comparator<Int> {
    public function compare(Int a, Int b): int {
        return a.getValue() - b.getValue();
    }
}

function main(): void {
    print("Testing Stream sortedWith with chaining:");

    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(8));
    list.add(new Int(3));
    list.add(new Int(12));
    list.add(new Int(5));
    list.add(new Int(15));
    list.add(new Int(2));
    list.add(new Int(9));

    print("Original:");
    for (Int num : list) {
        print(num);
    }

    // Filter even numbers, sort, then double them
    IntComparator comparator = new IntComparator();
    Stream<Int> stream = list.stream()
        .filter(x -> x.getValue() % 2 == 0)
        .sortedWith(comparator)
        .map(x -> new Int(x.getValue() * 2));

    Int[] result = stream.toArray();

    print("After filter(even), sortedWith, map(*2):");
    for (Int num : result) {
        print(num);
    }

    print("Count: " + result.length);
    print("Stream sortedWith chaining test passed!");
}

main();
