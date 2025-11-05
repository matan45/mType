// Test interface with methods having their own generic types
// @Script

import * from "../../lib/collections/List.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Container {
    function add<T>(T item): void;
    function get<T>(int index): T;
    function transform<T, R>(T item, Function<T, R> mapper): R;
}

class SimpleContainer implements Container {
    private List<Any> items;

    public constructor() {
        this.items = new List<Any>();
    }

    public function add<T>(T item): void {
        this.items.add(item);
    }

    public function get<T>(int index): T {
        return this.items.get(index);
    }

    public function transform<T, R>(T item, Function<T, R> mapper): R {
        return mapper(item);
    }
}

SimpleContainer container = new SimpleContainer();
container.add<String>("Hello");
container.add<Int>(42);

string str = container.get<String>(0);
int num = container.get<Int>(1);

int doubled = container.transform<Int, Int>(5, x -> x * 2);

print(str);
print(num);
print(doubled);  // Should print 10
