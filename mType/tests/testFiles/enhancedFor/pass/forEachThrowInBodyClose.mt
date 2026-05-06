// Test that an exception thrown in for-each body still allows the same collection
// to be iterated again afterwards (iterator must be cleaned up)
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing for-each throw in body:");

    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));
    list.add(new Int(3));
    list.add(new Int(4));

    try {
        for (Int x : list) {
            if (x.getValue() == 3) {
                throw new RuntimeException("body fail");
            }
            print(x);
        }
    } catch (RuntimeException e) {
        print("caught: " + e.getMessage());
    }

    print("second pass:");
    for (Int y : list) {
        print(y);
    }

    print("for-each throw in body test passed!");
}
main();
