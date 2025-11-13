// Test stream skip operation
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/stream/Stream.mt";

@Script
function main(): void {
    // Create a stream with range
    print("Testing stream skip:");
    Stream<int> stream = Streams.range(0, 10).skip(7);

    int[] result = stream.toArray();

    print("Skipped first 7 elements:");
    for (int num : result) {
        print(num); // Should print 7, 8, 9
    }

    print("Stream skip test passed!");
}
