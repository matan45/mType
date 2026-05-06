// Test that peek without a terminal operation produces no side effects
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Before peek:");

    Int[] data = [new Int(10), new Int(20), new Int(30)];
    Stream<Int> s = Streams::of<Int>(data).peek(v -> print("LEAK:" + v.getValue()));

    print("After peek (no terminal yet)");
    print("Stream peek not consumed test passed!");
}
main();
