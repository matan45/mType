// A stream pipeline whose lambda throws and is not surrounded by
// try/catch must surface the exception at the top level.
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    Int[] nums = [1, 2, 0, 4];
    Int[] result = Streams::of(nums).map(n -> {
        if (n.getValue() == 0) {
            throw new Exception("zero in stream");
        }
        return new Int(100 / n.getValue());
    }).toArray();
    print("len " + result.length);
}
main();
