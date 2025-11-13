// Test stream distinct operation
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/stream/Stream.mt";

@Script
function main(): void {
    // Create a stream with duplicates
    int[] numbers = [1, 2, 2, 3, 3, 3, 4, 4, 5];
    print("Testing stream distinct:");

    Stream<int> stream = Streams.of<int>(numbers).distinct();
    int[] result = stream.toArray();

    print("Distinct elements:");
    for (int num : result) {
        print(num); // Should print 1, 2, 3, 4, 5
    }

    print("Stream distinct test passed!");
}
