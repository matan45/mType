// Test single abstract method interface with lambda
// @Script

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Bool.mt";

interface Predicate<T> {
    function test(T value): bool;
}

interface Transformer<T, R> {
    function transform(T input): R;
}

class IntPredicate implements Predicate<Int> {
    public function test(Int value): bool {
        return value.getValue() % 2 == 0;
    }
}

class IntDoubleTransformer implements Transformer<Int, Int> {
    public function transform(Int input): Int {
        return new Int(input.getValue() * 2);
    }
}

class ArrayListFilter<T> {
    private ArrayList<T> items;

    public constructor(ArrayList<T> items) {
        this.items = items;
    }

    public function filter(Predicate<T> predicate): ArrayList<T> {
        ArrayList<T> result = new ArrayList<T>();
        int i = 0;
        while (i < this.items.size()) {
            T item = this.items.get(i);
            if (predicate.test(item)) {
                result.add(item);
            }
            i = i + 1;
        }
        return result;
    }

    public function map(Transformer<T, T> transformer): ArrayList<T> {
        ArrayList<T> result = new ArrayList<T>();
        int i = 0;
        while (i < this.items.size()) {
            T item = this.items.get(i);
            result.add(transformer.transform(item));
            i = i + 1;
        }
        return result;
    }
}

ArrayList<Int> numbers = new ArrayList<Int>();
numbers.add(new Int(1));
numbers.add(new Int(2));
numbers.add(new Int(3));
numbers.add(new Int(4));
numbers.add(new Int(5));

ArrayListFilter<Int> filter = new ArrayListFilter<Int>(numbers);

// Use predicate
IntPredicate evenPredicate = new IntPredicate();

ArrayList<Int> evens = filter.filter(evenPredicate);
print("Even numbers:");
int i = 0;
while (i < evens.size()) {
    print(evens.get(i).toString());
    i = i + 1;
}
