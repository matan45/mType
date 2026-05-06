// Edge: single-element collection — guards against off-by-one in hasNext/next.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayList<Int> a = new ArrayList<Int>();
    a.add(new Int(42));

    int count = 0;
    for (Int v : a) {
        print(v.getValue());
        count = count + 1;
    }
    print("c=" + count);
}

main();
