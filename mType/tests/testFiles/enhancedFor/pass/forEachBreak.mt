// Test break inside enhanced for-each terminates iteration and closes iterator
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing for-each break:");

    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));
    list.add(new Int(3));
    list.add(new Int(4));
    list.add(new Int(5));

    for (Int x : list) {
        if (x.getValue() == 3) {
            break;
        }
        print(x);
    }

    print("after break");
    for (Int y : list) {
        print(y);
    }

    print("for-each break test passed!");
}
main();
