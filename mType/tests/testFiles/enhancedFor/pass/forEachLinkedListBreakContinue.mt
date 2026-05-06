// Test break and continue inside for-each over LinkedList (distinct iterator class)
import * from "../../lib/collections/LinkedList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing for-each break on LinkedList:");

    LinkedList<Int> list = new LinkedList<Int>();
    list.add(new Int(10));
    list.add(new Int(20));
    list.add(new Int(30));
    list.add(new Int(40));
    list.add(new Int(50));

    for (Int x : list) {
        if (x.getValue() == 30) {
            break;
        }
        print(x);
    }

    print("Testing for-each continue on LinkedList:");
    for (Int y : list) {
        if (y.getValue() == 30) {
            continue;
        }
        print(y);
    }

    print("LinkedList break/continue test passed!");
}
main();
