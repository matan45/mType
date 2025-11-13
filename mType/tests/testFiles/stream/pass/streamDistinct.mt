// Test stream distinct operation
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    // Create a stream with duplicates
    Int[] numbers = [1, 2, 2, 3, 3, 3, 4, 4, 5];
    print("Testing stream distinct:");

    Stream<Int> stream = Streams::of(numbers).distinct();
    Int[] result = stream.toArray();

    print("Distinct elements:");
    for (Int num : result) {
        print(num); // Should print 1, 2, 3, 4, 5
    }

    print("Stream distinct test passed!");
}

main();
