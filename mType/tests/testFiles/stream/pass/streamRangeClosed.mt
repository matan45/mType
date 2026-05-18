// Test Streams.rangeClosed() factory - inclusive integer range
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing Streams.rangeClosed:");

    // Inclusive range [1, 5]
    Int[] inclusive = Streams::rangeClosed(1, 5).toArray();
    print("Length: " + inclusive.length);
    for (Int v : inclusive) {
        print(v);
    }

    // Single-element range: start == end
    Int[] single = Streams::rangeClosed(3, 3).toArray();
    print("Single length: " + single.length);
    for (Int v : single) {
        print(v);
    }

    // Sum via reduceWithIdentity on [1, 10]
    Int sum = Streams::rangeClosed(1, 10).reduceWithIdentity(new Int(0), (a, b) -> a.add(b));
    print("Sum 1..10: " + sum.getValue());

    print("Streams.rangeClosed test passed!");
}

main();
