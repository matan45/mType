// Edge: for-each over a ternary expression — the iterable is chosen at
// runtime. Each branch is a different ArrayList instance.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayList<Int> a = new ArrayList<Int>();
    a.add(new Int(1));
    a.add(new Int(2));

    ArrayList<Int> b = new ArrayList<Int>();
    b.add(new Int(10));
    b.add(new Int(20));
    b.add(new Int(30));

    bool pickA = false;
    int total = 0;
    for (Int v : pickA ? a : b) {
        total = total + v.getValue();
    }
    print("total=" + total);
}

main();

// Expected output:
// total=60
