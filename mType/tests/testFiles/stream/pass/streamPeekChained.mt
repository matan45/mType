// Test peek fires only for elements surviving filter (lazy semantics)
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing peek after filter:");

    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));
    list.add(new Int(3));
    list.add(new Int(4));

    Int[] result = list.stream()
        .filter(v -> v.getValue() > 1)
        .peek(v -> print("peek:" + v.getValue()))
        .map<Int>(v -> new Int(v.getValue() * 10))
        .toArray();

    print("result:");
    for (Int x : result) {
        print(x);
    }

    print("Stream peek chained test passed!");
}
main();
