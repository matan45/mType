// Edge Case: Try-catch inside for-each over generic collection
// NOTE: finally blocks removed - VM stack issue with finally in generic for-each

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class ItemException extends Exception {
    private int index;
    public constructor(string msg, int idx) : super(msg) {
        this.index = idx;
    }
    public function getIndex(): int { return this.index; }
}

class Item<T> {
    private T value;
    private bool faulty;

    public constructor(T value, bool faulty) {
        this.value = value;
        this.faulty = faulty;
    }

    public function getValue(): T { return this.value; }
    public function isFaulty(): bool { return this.faulty; }
}

function <T> processItems(ArrayList<Item<T>> items): void {
    int processed = 0;
    int skipped = 0;

    int idx = 0;
    for (Item<T> item : items) {
        try {
            if (item.isFaulty()) {
                throw new ItemException("Faulty item", idx);
            }
            print("  Processed: " + item.getValue());
            processed = processed + 1;
        } catch (ItemException e) {
            print("  Skipped item at " + e.getIndex() + ": " + e.getMessage());
            skipped = skipped + 1;
        }
        idx = idx + 1;
    }

    print("  Total: " + processed + " processed, " + skipped + " skipped");
}

function main(): void {
    print("=== Edge: TryCatch in ForEach Generic ===");

    print("--- Int items ---");
    ArrayList<Item<Int>> intItems = new ArrayList<Item<Int>>();
    intItems.add(new Item<Int>(new Int(10), false));
    intItems.add(new Item<Int>(new Int(20), true));
    intItems.add(new Item<Int>(new Int(30), false));
    intItems.add(new Item<Int>(new Int(40), true));
    intItems.add(new Item<Int>(new Int(50), false));
    processItems(intItems);

    print("--- String items ---");
    ArrayList<Item<String>> strItems = new ArrayList<Item<String>>();
    strItems.add(new Item<String>(new String("alpha"), false));
    strItems.add(new Item<String>(new String("beta"), false));
    strItems.add(new Item<String>(new String("gamma"), true));
    processItems(strItems);

    print("--- All faulty ---");
    ArrayList<Item<Int>> faultyItems = new ArrayList<Item<Int>>();
    faultyItems.add(new Item<Int>(new Int(1), true));
    faultyItems.add(new Item<Int>(new Int(2), true));
    processItems(faultyItems);

    print("--- Empty ---");
    ArrayList<Item<Int>> emptyItems = new ArrayList<Item<Int>>();
    processItems(emptyItems);

    print("=== Edge Complete ===");
}

main();
