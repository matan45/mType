// Edge Case: Stream pipeline converting between Box class types

import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Float.mt";

function main(): void {
    print("=== Edge: Stream Box Conversions ===");

    // Int -> String conversion via stream
    print("--- Int to String ---");
    Int[] numbers = [1, 2, 3, 4, 5];
    Stream<String> asStrings = Streams::of(numbers)
        .map(n -> new String("num-" + n.getValue()));
    String[] stringResults = asStrings.toArray();
    for (String s : stringResults) {
        print(s.getValue());
    }

    // Filter Int, map to String, filter String
    print("--- Chain Int -> String with filters ---");
    Stream<String> chained = Streams::of(numbers)
        .filter(n -> n.getValue() % 2 != 0)  // keep odd: 1,3,5
        .map(n -> new String("odd-" + n.getValue()));
    String[] chainedResults = chained.toArray();
    for (String s : chainedResults) {
        print(s.getValue());
    }

    // Int reduce
    print("--- Int reduce ---");
    Int product = Streams::of(numbers).reduceWithIdentity(1, (a, b) -> a * b);
    print("Product 1*2*3*4*5: " + product.getValue());

    // Int allMatch / anyMatch
    print("--- Match operations ---");
    bool allPositive = Streams::of(numbers).allMatch(n -> n.getValue() > 0);
    print("All positive: " + allPositive);

    bool anyGreaterThan4 = Streams::of(numbers).anyMatch(n -> n.getValue() > 4);
    print("Any > 4: " + anyGreaterThan4);

    bool anyGreaterThan10 = Streams::of(numbers).anyMatch(n -> n.getValue() > 10);
    print("Any > 10: " + anyGreaterThan10);

    // Distinct
    print("--- Distinct ---");
    Int[] withDups = [1, 2, 2, 3, 3, 3, 4];
    Stream<Int> distinct = Streams::of(withDups).distinct();
    Int[] distinctResult = distinct.toArray();
    print("Distinct count: " + distinctResult.length);
    for (Int n : distinctResult) {
        print("  " + n.getValue());
    }

    // Limit + Skip
    print("--- Limit and Skip ---");
    Int[] limited = Streams::of(numbers).limit(3).toArray();
    print("First 3:");
    for (Int n : limited) {
        print("  " + n.getValue());
    }

    Int[] skipped = Streams::of(numbers).skip(3).toArray();
    print("After skipping 3:");
    for (Int n : skipped) {
        print("  " + n.getValue());
    }

    print("=== Edge Complete ===");
}

main();
