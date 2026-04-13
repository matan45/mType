// Test: import {Symbol as Alias} from "source.mt" (source import with alias)
import {Int as MyInt} from "../../../lib/primitives/Int.mt";

class Wrapper {
    MyInt my;

    public constructor(MyInt v) {
        this.my = v;
    }

    public function get(): int {
        return this.my.getValue();
    }
}

Wrapper w = new Wrapper(new MyInt(42));
print(w.get());
print("Source import alias test passed");
