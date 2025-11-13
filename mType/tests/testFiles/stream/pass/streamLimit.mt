// Test stream limit operation
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/stream/Stream.mt";

@Script
function main(): void {
    // Create a stream with range
    print("Testing stream limit:");
    Stream<int> stream = Streams.range(0, 100).limit(5);

    int[] result = stream.toArray();

    print("Limited to 5 elements:");
    for (int num : result) {
        print(num); // Should print 0, 1, 2, 3, 4
    }

    print("Stream limit test passed!");
}
