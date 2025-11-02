// Test: Interface accepting T[] with lambdas
// @Script

interface ArrayProcessor<T> {
    map(arr: T[], transformer: (T) -> T) : T[];
    filter(arr: T[], predicate: (T) -> Bool) : T[];
    reduce(arr: T[], accumulator: (T, T) -> T, initial: T) : T;
}

class GenericArrayProcessor<T> : ArrayProcessor<T> {
    map(arr: T[], transformer: (T) -> T) : T[] {
        let result: T[] = [];
        let i: Int = 0;
        while (i < arr.length()) {
            result.push(transformer(arr[i]));
            i = i + 1;
        }
        return result;
    }

    filter(arr: T[], predicate: (T) -> Bool) : T[] {
        let result: T[] = [];
        let i: Int = 0;
        while (i < arr.length()) {
            if (predicate(arr[i])) {
                result.push(arr[i]);
            }
            i = i + 1;
        }
        return result;
    }

    reduce(arr: T[], accumulator: (T, T) -> T, initial: T) : T {
        let result = initial;
        let i: Int = 0;
        while (i < arr.length()) {
            result = accumulator(result, arr[i]);
            i = i + 1;
        }
        return result;
    }
}

main() : Void {
    let intProcessor: ArrayProcessor<Int> = new GenericArrayProcessor<Int>();

    let numbers: Int[] = [1, 2, 3, 4, 5];

    // Map: double each number
    let doubled = intProcessor.map(numbers, (n: Int) : Int => { return n * 2; });
    print("Doubled: " + doubled[0].toString() + ", " + doubled[1].toString());
    assert(doubled[0] == 2, "Should double first element");
    assert(doubled[4] == 10, "Should double last element");

    // Filter: only even numbers
    let evens = intProcessor.filter(numbers, (n: Int) : Bool => { return n % 2 == 0; });
    print("Evens count: " + evens.length().toString());
    assert(evens.length() == 2, "Should have 2 even numbers");

    // Reduce: sum
    let sum = intProcessor.reduce(numbers, (a: Int, b: Int) : Int => { return a + b; }, 0);
    print("Sum: " + sum.toString());
    assert(sum == 15, "Should sum to 15");

    print("Array interface test passed");
}
