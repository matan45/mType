// Test LinkedList removeFirst and removeLast update head/tail pointers correctly
import * from "../../lib/collections/LinkedList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing LinkedList removeFirst:");

    LinkedList<Int> a = new LinkedList<Int>();
    a.add(new Int(1));
    a.add(new Int(2));
    a.add(new Int(3));
    Int first = a.removeFirst();
    print("removed: " + first.getValue());
    print("size: " + a.size());
    for (Int x : a) {
        print(x);
    }

    print("Testing LinkedList removeLast:");
    LinkedList<Int> b = new LinkedList<Int>();
    b.add(new Int(4));
    b.add(new Int(5));
    b.add(new Int(6));
    Int last = b.removeLast();
    print("removed: " + last.getValue());
    print("size: " + b.size());
    for (Int x : b) {
        print(x);
    }

    print("LinkedList removeFirst/removeLast test passed!");
}
main();
