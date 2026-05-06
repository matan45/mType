// Test that hasNext() remains false after iterator is fully drained
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing hasNext after exhaustion:");

    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));

    Iterator<Int> iter = list.iterator();
    while (iter.hasNext()) {
        Int v = iter.next();
        print(v);
    }

    print("post-drain 1: " + iter.hasNext());
    print("post-drain 2: " + iter.hasNext());

    iter.close();

    print("hasNext after exhaustion test passed!");
}
main();
