// Test: iterator created after clear() yields no elements
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));
    list.add(new Int(3));
    list.clear();

    Iterator<Int> iter = list.iterator();
    print("hasNext after clear: " + iter.hasNext());
    iter.close();

    list.add(new Int(99));
    Iterator<Int> iter2 = list.iterator();
    while (iter2.hasNext()) {
        Int v = iter2.next();
        print(v.getValue());
    }
    iter2.close();
}
main();

// Expected output:
// hasNext after clear: false
// 99
