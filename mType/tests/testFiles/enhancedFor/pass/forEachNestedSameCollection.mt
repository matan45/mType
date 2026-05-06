// Test nested for-each over the SAME ArrayList instance — iterators must not share state
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing nested for-each same collection:");

    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));
    list.add(new Int(3));

    int total = 0;
    for (Int outer : list) {
        for (Int inner : list) {
            total = total + outer.getValue() * inner.getValue();
        }
    }
    print("total: " + total);

    print("nested for-each same collection test passed!");
}
main();
