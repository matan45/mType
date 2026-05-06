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

    ArrayList<Int> log = new ArrayList<Int>();

    Int[] result = list.stream()
        .filter(v -> v.getValue() > 1)
        .peek(v -> { log.add(v); return; })
        .map<Int>(v -> new Int(v.getValue() * 10))
        .toArray();

    print("log size: " + log.size());
    for (int i = 0; i < log.size(); i++) {
        print(log.get(i));
    }
    print("result:");
    for (Int x : result) {
        print(x);
    }

    print("Stream peek chained test passed!");
}
main();
