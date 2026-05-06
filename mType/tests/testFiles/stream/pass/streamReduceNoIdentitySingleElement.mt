// Test reduce on single-element stream returns that element without invoking accumulator
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing reduce single element:");

    Int[] data = [new Int(42)];
    Int result = Streams::of<Int>(data).reduce((a, b) -> {
        print("accumulator should not be called");
        return new Int(a.getValue() + b.getValue());
    });
    print("result: " + result.getValue());

    print("Stream reduce single element test passed!");
}
main();
