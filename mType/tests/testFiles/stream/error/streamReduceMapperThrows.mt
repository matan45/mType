// Test: exception inside reduce accumulator must propagate
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/Int.mt";

Int[] data = [new Int(1), new Int(2), new Int(3)];
Int sum = Streams::of<Int>(data).reduce((a, b) -> {
    throw new RuntimeException("boom from reduce");
    return a;
});
print("should not reach: " + sum.getValue());
