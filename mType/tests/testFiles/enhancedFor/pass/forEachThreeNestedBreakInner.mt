// Edge: 3-level nested for-each, inner-most loop uses break. The break must
// only exit the innermost loop and leave outer iterators intact.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayList<Int> a = new ArrayList<Int>();
    a.add(new Int(1));
    a.add(new Int(2));

    ArrayList<Int> b = new ArrayList<Int>();
    b.add(new Int(10));
    b.add(new Int(20));

    ArrayList<Int> c = new ArrayList<Int>();
    c.add(new Int(100));
    c.add(new Int(200));
    c.add(new Int(300));

    for (Int x : a) {
        for (Int y : b) {
            for (Int z : c) {
                if (z.getValue() == 200) {
                    break;
                }
                print("" + x.getValue() + "," + y.getValue() + "," + z.getValue());
            }
        }
    }
}

main();
