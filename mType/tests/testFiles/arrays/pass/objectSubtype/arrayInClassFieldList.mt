// MYT-281: a class field of type `List<Object>` accepts arrays as
// elements. Combines field assignment and method-parameter widening on
// the same path.
import * from "../../../lib/collections/ArrayList.mt";

class Bag {
    public ArrayList<Object> items;

    constructor() {
        this.items = new ArrayList<Object>();
    }

    public function addItem(Object o): void {
        this.items.add(o);
    }

    public function count(): int {
        return this.items.size();
    }
}

print("Testing array in class field List<Object>");

Bag bag = new Bag();
bag.addItem(new int[2]);
bag.addItem(new string[3]);
bag.addItem(new bool[1]);

print("Count: " + bag.count());

Object first = bag.items.get(0);
int[] firstAsInt = (int[])first;
print("First length: " + firstAsInt.length);

print("Done");
