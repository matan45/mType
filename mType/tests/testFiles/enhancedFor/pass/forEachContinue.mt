// Test continue inside enhanced for-each skips to the next element
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing for-each continue:");

    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));
    list.add(new Int(3));
    list.add(new Int(4));
    list.add(new Int(5));

    for (Int x : list) {
        if (x.getValue() % 2 == 0) {
            continue;
        }
        print(x);
    }

    print("for-each continue test passed!");
}
main();
