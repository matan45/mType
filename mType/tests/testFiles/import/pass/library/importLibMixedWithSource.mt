// Test: mixing import lib with regular source imports in the same file
import * from "../../../lib/primitives/Int.mt";

class LocalBox<T> {
    T item;
    public constructor(T item) {
        this.item = item;
    }
    public function getItem(): T {
        return this.item;
    }
}

LocalBox<Int> box = new LocalBox<Int>(new Int(99));
print("Box contains: " + box.getItem());
print("Mixed import test passed");
