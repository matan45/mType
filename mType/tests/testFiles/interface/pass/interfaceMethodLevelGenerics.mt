// Test interface with methods having their own generic types
// @Script

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Container {
    function <T> add(T item): void;
    function <T> get(int index): T;
    function <T, R> transform(T item, Function<T, R> mapper): R;
}


class SimpleContainer implements Container {
    private ArrayList<Int> items;

    public constructor() {
        this.items = new ArrayList<Int>();
    }

    public function <T> add(T item): void {
        this.items.add(item);
    }

    public function <T> get(int index): T {
        return this.items.get(index);
    }

    public function <T, R> transform(T item, Function<T, R> mapper): R {
        return mapper.apply(item);
    }
}

SimpleContainer container = new SimpleContainer();
container.add<Int>(new Int(42));

Int num = container.get<Int>(0);

Int doubled = container.transform<Int, Int>(new Int(5), x -> new Int(x.getValue() * 2));

print(num.toString());
print(doubled.toString());  // Should print 10
