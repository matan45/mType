// Test generic lambda inference with interfaces
// @Script

interface Mapper<T, R> {
    func map(input: T): R;
}

interface Filter<T> {
    func test(input: T): Bool;
}

class Pipeline<T> {
    var items: Array<T>;

    func init(items: Array<T>) {
        this.items = items;
    }

    func filter(predicate: Filter<T>): Pipeline<T> {
        var filtered = new Array<T>();
        var i = 0;
        while (i < this.items.size()) {
            var item = this.items.get(i);
            if (predicate.test(item)) {
                filtered.add(item);
            }
            i = i + 1;
        }
        return new Pipeline<T>(filtered);
    }

    func map<R>(mapper: Mapper<T, R>): Pipeline<R> {
        var mapped = new Array<R>();
        var i = 0;
        while (i < this.items.size()) {
            var item = this.items.get(i);
            mapped.add(mapper.map(item));
            i = i + 1;
        }
        return new Pipeline<R>(mapped);
    }

    func collect(): Array<T> {
        return this.items;
    }
}

class LambdaMapper<T, R> implements Mapper<T, R> {
    var fn: func(T): R;

    func init(fn: func(T): R) {
        this.fn = fn;
    }

    func map(input: T): R {
        return this.fn(input);
    }
}

class LambdaFilter<T> implements Filter<T> {
    var fn: func(T): Bool;

    func init(fn: func(T): Bool) {
        this.fn = fn;
    }

    func test(input: T): Bool {
        return this.fn(input);
    }
}

var numbers = new Array<Int>();
numbers.add(1);
numbers.add(2);
numbers.add(3);
numbers.add(4);
numbers.add(5);

var pipeline = new Pipeline<Int>(numbers);

// Filter even numbers
var evenFilter = new LambdaFilter<Int>(func(x: Int): Bool {
    return x % 2 == 0;
});

// Double the numbers
var doubleMapper = new LambdaMapper<Int, Int>(func(x: Int): Int {
    return x * 2;
});

var result = pipeline
    .filter(evenFilter)
    .map<Int>(doubleMapper)
    .collect();

print("Results:");
var i = 0;
while (i < result.size()) {
    print(result.get(i));
    i = i + 1;
}
