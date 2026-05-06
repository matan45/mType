// Test LinkedList remove last remaining element resets both head and tail
import * from "../../lib/collections/LinkedList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing LinkedList remove single element:");

    LinkedList<Int> list = new LinkedList<Int>();
    list.add(new Int(99));
    list.removeAt(0);

    print("size: " + list.size());
    int iterations = 0;
    for (Int x : list) {
        iterations = iterations + 1;
    }
    print("iterations: " + iterations);

    list.add(new Int(7));
    print("after re-add size: " + list.size());
    for (Int x : list) {
        print(x);
    }

    print("LinkedList remove single element test passed!");
}
main();
