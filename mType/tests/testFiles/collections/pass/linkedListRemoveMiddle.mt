// Test LinkedList removeAt for a middle index re-links neighbors correctly
import * from "../../lib/collections/LinkedList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing LinkedList remove middle:");

    LinkedList<Int> list = new LinkedList<Int>();
    list.add(new Int(10));
    list.add(new Int(20));
    list.add(new Int(30));
    list.add(new Int(40));
    list.add(new Int(50));

    list.removeAt(2);

    print("size: " + list.size());
    for (Int x : list) {
        print(x);
    }

    print("indexOf 40: " + list.indexOf(new Int(40)));
    print("indexOf 30: " + list.indexOf(new Int(30)));

    print("LinkedList remove middle test passed!");
}
main();
