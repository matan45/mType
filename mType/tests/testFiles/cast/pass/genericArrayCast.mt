// Test: cast a generic class instance that carries a T[] field; access the
// array elements through the upcast reference to prove the cast is real.
import * from "../../lib/primitives/Int.mt";

class Container<T> {
    public T[] items;
    public constructor(T[] arr) { this.items = arr; }
    public function get(int i): T { return this.items[i]; }
}

class Box<T> extends Container<T> {
    public constructor(T[] arr) : super(arr) {}
}

Int[] nums = new Int[3];
nums[0] = new Int(10);
nums[1] = new Int(20);
nums[2] = new Int(30);

Box<Int> b = new Box<Int>(nums);
Container<Int> c = (Container<Int>)b;
print(c.get(0).toString());
print(c.get(1).toString());
print(c.get(2).toString());

// Expected output:
// 10
// 20
// 30
