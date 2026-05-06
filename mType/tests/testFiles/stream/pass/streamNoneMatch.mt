// Test noneMatch on three cases: all-fail, partial, empty
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing noneMatch:");

    Int[] data1 = [new Int(1), new Int(2), new Int(3)];
    bool r1 = Streams::of<Int>(data1).noneMatch(v -> v.getValue() > 10);
    print("none > 10 in [1,2,3]: " + r1);

    Int[] data2 = [new Int(1), new Int(2), new Int(11)];
    bool r2 = Streams::of<Int>(data2).noneMatch(v -> v.getValue() > 10);
    print("none > 10 in [1,2,11]: " + r2);

    Int[] data3 = new Int[0];
    bool r3 = Streams::of<Int>(data3).noneMatch(v -> v.getValue() > 10);
    print("none > 10 in []: " + r3);

    print("Stream noneMatch test passed!");
}
main();
