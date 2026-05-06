// Test all terminal operations on an empty stream
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing terminals on empty stream:");

    Int[] empty1 = new Int[0];
    int c = Streams::of<Int>(empty1).count();
    print("count: " + c);

    Int[] empty2 = new Int[0];
    bool any = Streams::of<Int>(empty2).anyMatch(v -> true);
    print("anyMatch: " + any);

    Int[] empty3 = new Int[0];
    bool all = Streams::of<Int>(empty3).allMatch(v -> false);
    print("allMatch: " + all);

    Int[] empty4 = new Int[0];
    bool none = Streams::of<Int>(empty4).noneMatch(v -> true);
    print("noneMatch: " + none);

    Int[] empty5 = new Int[0];
    Int[] arr = Streams::of<Int>(empty5).toArray();
    print("toArray length: " + arr.length);

    Int[] empty6 = new Int[0];
    print("forEach output:");
    Streams::of<Int>(empty6).forEach(v -> print("UNEXPECTED:" + v.getValue()));
    print("forEach done");

    print("Stream empty terminals test passed!");
}
main();
