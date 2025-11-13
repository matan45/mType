// Test basic stream operations: filter, map, toArray
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/functional/Predicate.mt";
import * from "../../lib/functional/Function.mt";

// Simple predicate implementation for filtering even numbers
class IsEvenPredicate implements Predicate<int> {
    public function test(int value): bool {
        return value % 2 == 0;
    }
}

// Simple function implementation for doubling values
class DoubleFunction implements Function<int, int> {
    public function apply(int value): int {
        return value * 2;
    }
}

@Script
function main(): void {
    // Create a list
    ArrayList<int> list = new ArrayList<int>();
    list.add(1);
    list.add(2);
    list.add(3);
    list.add(4);
    list.add(5);

    // Test filter and map
    print("Testing stream filter and map:");
    Stream<int> stream = list.stream()
        .filter(new IsEvenPredicate())
        .map<int>(new DoubleFunction());

    int[] result = stream.toArray();

    print("Filtered even numbers and doubled:");
    for (int num : result) {
        print(num); // Should print 4, 8
    }

    print("Stream basic operations test passed!");
}
