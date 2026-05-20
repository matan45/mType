// Edge: loop body mutates a field on each element. The collection structure
// is untouched, only per-element state changes. Iteration must complete.
import * from "../../lib/collections/ArrayList.mt";

class Counter {
    public int value;
    constructor(int v) {
        this.value = v;
    }
}

function main(): void {
    ArrayList<Counter> list = new ArrayList<Counter>();
    list.add(new Counter(1));
    list.add(new Counter(2));
    list.add(new Counter(3));

    for (Counter c : list) {
        c.value = c.value * 10;
    }

    int total = 0;
    for (Counter c : list) {
        total = total + c.value;
    }
    print("total=" + total);
}

main();

// Expected output:
// total=60
