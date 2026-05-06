// Test stream flatMap with non-empty, empty, and non-empty inner streams
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing flatMap with mixed inner streams:");

    ArrayList<ArrayList<Int>> outer = new ArrayList<ArrayList<Int>>();

    ArrayList<Int> a = new ArrayList<Int>();
    a.add(new Int(1));
    a.add(new Int(2));
    outer.add(a);

    ArrayList<Int> b = new ArrayList<Int>();
    outer.add(b);

    ArrayList<Int> c = new ArrayList<Int>();
    c.add(new Int(3));
    c.add(new Int(4));
    c.add(new Int(5));
    outer.add(c);

    Stream<Int> result = outer.stream().flatMap<Int>(inner -> Streams::fromIterable<Int>(inner));
    Int[] arr = result.toArray();
    for (Int x : arr) {
        print(x);
    }

    print("Stream flatMap test passed!");
}
main();
