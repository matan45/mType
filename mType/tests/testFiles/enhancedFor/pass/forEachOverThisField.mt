// Edge: collection expression is `this.field`. Type inference for the
// collection element must work through member access, not just locals.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

class Holder {
    public ArrayList<Int> items;

    public constructor() {
        this.items = new ArrayList<Int>();
        this.items.add(new Int(10));
        this.items.add(new Int(20));
    }

    public function show(): void {
        for (Int v : this.items) {
            print(v.getValue());
        }
    }
}

function main(): void {
    Holder h = new Holder();
    h.show();
}

main();
