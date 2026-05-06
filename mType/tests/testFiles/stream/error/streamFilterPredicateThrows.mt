// Test: exception thrown inside filter predicate must propagate through terminal op
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/Int.mt";

Int[] data = [new Int(1), new Int(2), new Int(3)];
int c = Streams::of<Int>(data).filter(v -> {
    throw new RuntimeException("boom from filter");
    return false;
}).count();
print("should not reach: " + c);
