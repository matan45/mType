// Test ArrayList correctness across many internal resizes
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing ArrayList resize many adds:");

    ArrayList<Int> list = new ArrayList<Int>();
    for (int i = 0; i < 100; i++) {
        list.add(new Int(i));
    }
    print("size: " + list.size());

    int errors = 0;
    for (int i = 0; i < 100; i++) {
        Int v = list.get(i);
        if (v.getValue() != i) {
            errors = errors + 1;
        }
    }
    print("errors: " + errors);
    print("first: " + list.get(0).getValue());
    print("last: " + list.get(99).getValue());

    print("ArrayList resize many adds test passed!");
}
main();
