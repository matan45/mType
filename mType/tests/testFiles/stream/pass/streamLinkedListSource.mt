// Test stream pipeline starting from LinkedList source
import * from "../../lib/collections/LinkedList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing stream from LinkedList:");

    LinkedList<Int> list = new LinkedList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));
    list.add(new Int(3));
    list.add(new Int(4));
    list.add(new Int(5));

    Int[] result = list.stream()
        .filter(v -> v.getValue() % 2 == 0)
        .map<Int>(v -> new Int(v.getValue() * 3))
        .toArray();

    for (Int x : result) {
        print(x);
    }

    print("Stream LinkedList source test passed!");
}
main();
