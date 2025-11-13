// Test stream allMatch operation
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/primitives/Int.mt";


function main(): void {
    // Create a stream
    Int[] numbers = [1, 2, 3, 4, 5];
    print("Testing stream allMatch:");

    bool allPositive = Streams::of(numbers).allMatch(value-> value > 0);

    if (allPositive) {
        print("All numbers are positive");
    }

    print("Stream allMatch test passed!");
}

main();
