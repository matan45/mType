// Test: a lambda passed to a stream operation throws on a sentinel
// element. The exception propagates out of the terminal op and is
// caught by the surrounding try.
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    Int[] nums = [1, 2, 0, 4];
    try {
        Int[] result = Streams::of(nums).map(n -> {
            if (n.getValue() == 0) {
                throw new Exception("zero in stream");
            }
            return new Int(100 / n.getValue());
        }).toArray();
        print("size " + result.length);
    } catch (Exception e) {
        print("stream caught: " + e.getMessage());
    }
    print("after try");
}
main();
