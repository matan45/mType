// Test single abstract method interface with lambda
// @Script

interface Predicate<T> {
    func test(value: T): Bool;
}

interface Transformer<T, R> {
    func transform(input: T): R;
}

class ListFilter<T> {
    var items: Array<T>;

    func init(items: Array<T>) {
        this.items = items;
    }

    func filter(predicate: Predicate<T>): Array<T> {
        var result = new Array<T>();
        var i = 0;
        while (i < this.items.size()) {
            var item = this.items.get(i);
            if (predicate.test(item)) {
                result.add(item);
            }
            i = i + 1;
        }
        return result;
    }

    func map<R>(transformer: Transformer<T, R>): Array<R> {
        var result = new Array<R>();
        var i = 0;
        while (i < this.items.size()) {
            var item = this.items.get(i);
            result.add(transformer.transform(item));
            i = i + 1;
        }
        return result;
    }
}

// Lambda implementation of Predicate
class LambdaPredicate<T> implements Predicate<T> {
    var fn: func(T): Bool;

    func init(fn: func(T): Bool) {
        this.fn = fn;
    }

    func test(value: T): Bool {
        return this.fn(value);
    }
}

var numbers = new Array<Int>();
numbers.add(1);
numbers.add(2);
numbers.add(3);
numbers.add(4);
numbers.add(5);

var filter = new ListFilter<Int>(numbers);

// Use lambda as predicate
var evenPredicate = new LambdaPredicate<Int>(func(x: Int): Bool {
    return x % 2 == 0;
});

var evens = filter.filter(evenPredicate);
print("Even numbers:");
var i = 0;
while (i < evens.size()) {
    print(evens.get(i));
    i = i + 1;
}
