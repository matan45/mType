// Edge Case: Stream operations on empty collections

import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("=== Edge: Stream Empty Array ===");

    // Empty Int array stream
    print("--- Empty array stream ---");
    Int[] empty = new Int[0];
    Stream<Int> emptyStream = Streams::of(empty);

    Int[] result = emptyStream.toArray();
    print("Length: " + result.length);

    // Filter that removes everything
    print("--- Filter to empty ---");
    Int[] numbers = [1, 2, 3, 4, 5];
    Stream<Int> filtered = Streams::of(numbers).filter(n -> n.getValue() > 100);
    Int[] filteredResult = filtered.toArray();
    print("After filter: " + filteredResult.length);

    // Count on empty
    print("--- Count empty ---");
    int count = Streams::of(empty).count();
    print("Count: " + count);

    // Chained operations on empty
    print("--- Chain on empty ---");
    Int[] chained = Streams::of(empty)
        .filter(n -> n.getValue() > 0)
        .map(n -> new Int(n.getValue() * 2))
        .toArray();
    print("Chained result length: " + chained.length);

    // Single element stream
    print("--- Single element ---");
    Int[] single = [42];
    Stream<Int> singleStream = Streams::of(single);
    Int[] singleResult = singleStream.toArray();
    print("Single length: " + singleResult.length);
    print("Single value: " + singleResult[0].getValue());

    // Reduce on non-empty
    print("--- Reduce non-empty ---");
    Int sum = Streams::of(numbers).reduceWithIdentity(0, (a, b) -> a + b);
    print("Sum: " + sum.getValue());

    // Map on empty
    print("--- Map on empty ---");
    Int[] mapped = Streams::of(empty).map(n -> new Int(n.getValue() + 1)).toArray();
    print("Mapped empty length: " + mapped.length);

    // Empty ArrayList stream
    print("--- Empty ArrayList stream ---");
    ArrayList<Int> emptyList = new ArrayList<Int>();
    Stream<Int> listStream = emptyList.stream();
    Int[] listResult = listStream.toArray();
    print("Empty list stream length: " + listResult.length);

    print("=== Edge Complete ===");
}

main();
