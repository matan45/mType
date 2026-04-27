// Combo 12: Box Classes + Stream API + Lambda Pipeline

import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

function main(): void {
    print("=== Combo 12: Box Classes + Stream + Lambda ===");

    // Int stream pipeline
    print("--- Int stream pipeline ---");
    Int[] numbers = [new Int(1), new Int(2), new Int(3), new Int(4), new Int(5), new Int(6), new Int(7), new Int(8), new Int(9), new Int(10)];

    // Filter even, then double
    Stream<Int> evenDoubled = Streams::of(numbers)
        .filter(n -> n.getValue() % 2 == 0)
        .map(n -> new Int(n.getValue() * 2));

    Int[] results = evenDoubled.toArray();
    for (Int r : results) {
        print("  " + r.getValue());
    }

    // Reduce: sum
    print("--- Reduce sum ---");
    Int sum = Streams::of(numbers)
        .reduceWithIdentity(0, (a, b) -> a + b);
    print("Sum 1-10: " + sum.getValue());

    // Count
    print("--- Count ---");
    int oddCount = Streams::of(numbers)
        .filter(n -> n.getValue() % 2 != 0)
        .count();
    print("Odd count: " + oddCount);

    // String stream pipeline - avoid chained calls in lambda
    print("--- String stream ---");
    String[] words = [
        new String("hello"),
        new String("world"),
        new String("foo"),
        new String("bar"),
        new String("mtype")
    ];

    // Filter words longer than 3 chars
    Stream<String> longWords = Streams::of(words)
        .filter(w -> w.getValue().length() > 3);

    String[] filtered = longWords.toArray();
    for (String w : filtered) {
        print("  " + w.getValue());
    }

    // Chain: filter + count
    print("--- Chained operations ---");
    int count = Streams::of(numbers)
        .filter(n -> n.getValue() > 3)
        .filter(n -> n.getValue() < 8)
        .count();
    print("Numbers between 3 and 8: " + count);

    // Map to different type
    print("--- Map to strings ---");
    Stream<String> asStrings = Streams::of(numbers)
        .filter(n -> n.getValue() <= 3)
        .map(n -> new String("item-" + n.getValue()));

    String[] stringResults = asStrings.toArray();
    for (String s : stringResults) {
        print("  " + s.getValue());
    }

    print("=== Combo 12 Complete ===");
}

main();
