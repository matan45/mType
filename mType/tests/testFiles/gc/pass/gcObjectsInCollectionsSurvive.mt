// Test: objects reachable only through a collection's backing store survive
// allocation-pressure GC cycles (the map itself is the only root).
import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Garbage {
    private Garbage? next;
    constructor() {
        this.next = null;
    }
    public function setNext(Garbage? n): void { this.next = n; }
}

function churn(): void {
    for (int i = 0; i < 20000; i++) {
        Garbage g = new Garbage();
        Garbage h = new Garbage();
        g.setNext(h);
        h.setNext(g);
    }
}

function main(): void {
    HashMap<Int, String> map = new HashMap<Int, String>();
    for (int i = 0; i < 50; i++) {
        map.put(new Int(i), new String("value-" + i));
    }

    churn();

    int hits = 0;
    for (int i = 0; i < 50; i++) {
        String v = map.get(new Int(i));
        if (v != null) {
            hits = hits + 1;
        }
    }
    print("size: " + map.size());
    print("hits: " + hits);
    print("Objects in collections survive GC test passed!");
}
main();
