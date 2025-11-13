// Test stream allMatch operation
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/functional/Predicate.mt";

class PositivePredicate implements Predicate<int> {
    public function test(int value): bool {
        return value > 0;
    }
}

@Script
function main(): void {
    // Create a stream
    int[] numbers = [1, 2, 3, 4, 5];
    print("Testing stream allMatch:");

    bool allPositive = Streams.of<int>(numbers).allMatch(new PositivePredicate());

    if (allPositive) {
        print("All numbers are positive");
    }

    print("Stream allMatch test passed!");
}
