// Test: LinkedList.contains() and size() after a mixed add/remove sequence
import * from "../../lib/collections/LinkedList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    LinkedList<Int> list = new LinkedList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));
    list.add(new Int(3));
    list.add(new Int(4));
    print("size after adds: " + list.size());
    print("contains 3: " + list.contains(new Int(3)));

    list.remove(new Int(2));
    print("size after remove 2: " + list.size());
    print("contains 2: " + list.contains(new Int(2)));
    print("contains 3: " + list.contains(new Int(3)));

    list.removeFirst();
    list.removeLast();
    print("size after removeFirst+removeLast: " + list.size());
    print("first: " + list.first().getValue());
    print("contains 1: " + list.contains(new Int(1)));
    print("contains 4: " + list.contains(new Int(4)));
}
main();

// Expected output:
// size after adds: 4
// contains 3: true
// size after remove 2: 3
// contains 2: false
// contains 3: true
// size after removeFirst+removeLast: 1
// first: 3
// contains 1: false
// contains 4: false
