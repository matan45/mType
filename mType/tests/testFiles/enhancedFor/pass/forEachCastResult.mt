// Edge: for-each over the result of a cast expression. The iterable arrives
// typed as the base Object, is cast back to its concrete generic shape, and
// iterated.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(5));
    list.add(new Int(7));
    list.add(new Int(11));

    Object opaque = list;

    int sum = 0;
    for (Int v : (ArrayList<Int>) opaque) {
        sum = sum + v.getValue();
    }
    print("sum=" + sum);
}

main();

// Expected output:
// sum=23
