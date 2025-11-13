// Test stream reduce operation
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";


function main(): void {
    // Create a stream from array
    Int[] numbers = [1, 2, 3, 4, 5];
    Stream<Int> stream = Streams::of(numbers);

    // Test reduce with identity
    print("Testing stream reduce:");
    Int sum = stream.reduceWithIdentity(0, (left,right)-> left + right);

    print("Sum of 1+2+3+4+5 = " + sum); // Should be 15

    print("Stream reduce test passed!");
}
main();
