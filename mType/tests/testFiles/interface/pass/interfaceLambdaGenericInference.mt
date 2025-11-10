// Test generic lambda inference with interfaces
// @Script

import * from "../../lib/collections/List.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Bool.mt";

interface Mapper<T, R> {
    function map(T input): R;
}

interface Filter<T> {
    function test(T input): bool;
}

class IntMapper implements Mapper<Int, Int> {
    public function map(Int input): Int {
        return new Int(input.getValue() * 2);
    }
}

class IntFilter implements Filter<Int> {
    public function test(Int input): bool {
        return input.getValue() % 2 == 0;
    }
}

class Pipeline<T> {
    private List<T> items;

    public constructor(List<T> items) {
        this.items = items;
    }

    public function filter(Filter<T> predicate): Pipeline<T> {
        List<T> filtered = new List<T>();
        int i = 0;
        while (i < this.items.size()) {
            T item = this.items.get(i);
            if (predicate.test(item)) {
                filtered.add(item);
            }
            i = i + 1;
        }
        return new Pipeline<T>(filtered);
    }

    public function <R> map(Mapper<T, R> mapper): Pipeline<R> {
        List<R> mapped = new List<R>();
        int i = 0;
        while (i < this.items.size()) {
            T item = this.items.get(i);
            mapped.add(mapper.map(item));
            i = i + 1;
        }
        return new Pipeline<R>(mapped);
    }

    public function collect(): List<T> {
        return this.items;
    }
}

List<Int> numbers = new List<Int>();
numbers.add(new Int(1));
numbers.add(new Int(2));
numbers.add(new Int(3));
numbers.add(new Int(4));
numbers.add(new Int(5));

Pipeline<Int> pipeline = new Pipeline<Int>(numbers);

// Filter even numbers
IntFilter evenFilter = new IntFilter();

// Double the numbers
IntMapper doubleMapper = new IntMapper();

Pipeline<Int> result = pipeline
    .filter(evenFilter)
    .map<Int>(doubleMapper);

List<Int> resultList = result.collect();

print("Results:");
int i = 0;
while (i < resultList.size()) {
    print(resultList.get(i).toString());
    i = i + 1;
}
