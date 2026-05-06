// Test: exception thrown inside map function must propagate
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/Int.mt";

Int[] data = [new Int(1), new Int(2), new Int(3)];
Int[] arr = Streams::of<Int>(data).map<Int>(v -> {
    throw new RuntimeException("boom from map");
    return v;
}).toArray();
print("should not reach: " + arr.length);
