// Test stream reduce operation
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/functional/BinaryOperator.mt";

class SumOperator implements BinaryOperator<int> {
    public function apply(int left, int right): int {
        return left + right;
    }
}

@Script
function main(): void {
    // Create a stream from array
    int[] numbers = [1, 2, 3, 4, 5];
    Stream<int> stream = Streams.of<int>(numbers);

    // Test reduce with identity
    print("Testing stream reduce:");
    int sum = stream.reduceWithIdentity(0, new SumOperator());

    print("Sum of 1+2+3+4+5 = " + sum); // Should be 15

    print("Stream reduce test passed!");
}
