// Test that peek interleaves with terminal forEach (lazy per-element pipeline)
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing peek vs forEach order:");

    Int[] data = [new Int(1), new Int(2), new Int(3)];
    Streams::of<Int>(data)
        .peek(v -> { print("P:" + v.getValue()); return; })
        .forEach(v -> print("F:" + v.getValue()));

    print("Stream peek interleave test passed!");
}
main();
