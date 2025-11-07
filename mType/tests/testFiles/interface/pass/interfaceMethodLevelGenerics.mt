// Test interface with methods having their own generic types
// @Script

import * from "../../lib/collections/List.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Container {
    function <T> add(T item): void;
    function <T> get(int index): T;
    function <T, R> transform(T item, Function<T, R> mapper): R;
}

interface Function<T,R>{
	function map(T value): R;
}

class SimpleContainer implements Container {
    private List<Int> items;

    public constructor() {
        this.items = new List<Int>();
    }

    public function <T> add(T item): void {
        this.items.add(item);
    }

    public function <T> get(int index): T {
        return this.items.get(index);
    }

    public function <T, R> transform(T item, Function<T, R> mapper): R {
        return mapper.map(item);
    }
}

SimpleContainer container = new SimpleContainer();
container.add<Int>(new Int(42));

Int num = container.get<Int>(0);

Int doubled = container.transform<Int, Int>(new Int(5), x -> new Int(x.getValue() * 2));

print(num.toString());
print(doubled.toString());  // Should print 10
