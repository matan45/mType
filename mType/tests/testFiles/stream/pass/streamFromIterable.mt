// Test Streams.fromIterable() factory - builds Stream from any Iterable
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/collections/HashSet.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/functional/Comparator.mt";
import * from "../../lib/primitives/Int.mt";

class IntAsc implements Comparator<Int> {
    public function compare(Int a, Int b): int {
        return a.getValue() - b.getValue();
    }
}

function main(): void {
    print("Testing Streams.fromIterable:");

    // Build from an ArrayList
    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(10));
    list.add(new Int(20));
    list.add(new Int(30));
    Int[] fromList = Streams::fromIterable<Int>(list).toArray();
    print("List length: " + fromList.length);
    for (Int v : fromList) {
        print(v);
    }

    // Build from a HashSet (canonicalize order via sortedWith)
    HashSet<Int> set = new HashSet<Int>();
    set.add(7);
    set.add(3);
    set.add(11);
    Int[] fromSet = Streams::fromIterable<Int>(set).sortedWith(new IntAsc()).toArray();
    print("Set length: " + fromSet.length);
    for (Int v : fromSet) {
        print(v);
    }

    print("Streams.fromIterable test passed!");
}

main();
