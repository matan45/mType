// Test stream skip operation
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";


function main(): void {
    // Create a stream with range
    print("Testing stream skip:");
    Stream<Int> stream = Streams::range(0, 10).skip(7);

    Int[] result = stream.toArray();

    print("Skipped first 7 elements:");
    for (Int num : result) {
        print(num); // Should print 7, 8, 9
    }

    print("Stream skip test passed!");
}
main();
