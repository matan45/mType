// Test reduce without identity returns sum of all elements
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing reduce without identity:");

    Int[] data = [new Int(1), new Int(2), new Int(3), new Int(4)];
    Int sum = Streams::of<Int>(data).reduce((a, b) -> new Int(a.getValue() + b.getValue()));
    print("sum: " + sum.getValue());

    print("Stream reduce no-identity test passed!");
}
main();
