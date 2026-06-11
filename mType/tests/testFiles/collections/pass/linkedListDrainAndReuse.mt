// Test: draining a LinkedList to empty (head == tail == null) and then
// re-adding must rebuild head/tail pointers correctly.
import * from "../../lib/collections/LinkedList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing LinkedList drain and reuse:");

    LinkedList<Int> list = new LinkedList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));
    list.add(new Int(3));

    while (list.size() > 0) {
        Int v = list.removeFirst();
        print("drained: " + v.getValue());
    }
    print("size after drain: " + list.size());

    list.add(new Int(10));
    list.add(new Int(20));
    print("size after reuse: " + list.size());
    for (Int x : list) {
        print(x);
    }

    Int last = list.removeLast();
    print("removed last: " + last.getValue());
    Int first = list.removeFirst();
    print("removed first: " + first.getValue());
    print("final size: " + list.size());

    print("LinkedList drain and reuse test passed!");
}
main();
