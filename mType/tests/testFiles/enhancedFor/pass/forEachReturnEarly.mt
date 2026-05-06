// Test return from inside enhanced for-each (must close iterator before unwind)
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function findFirst(ArrayList<Int> data, int target): int {
    for (Int x : data) {
        if (x.getValue() == target) {
            return x.getValue();
        }
    }
    return -1;
}

function main(): void {
    print("Testing for-each return early:");

    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(3));
    list.add(new Int(5));
    list.add(new Int(7));
    list.add(new Int(11));

    int found = findFirst(list, 7);
    print("found: " + found);

    for (Int y : list) {
        print(y);
    }

    print("for-each return early test passed!");
}
main();
