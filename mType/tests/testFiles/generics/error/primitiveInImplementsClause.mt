// MYT-360: primitive type rejected in `implements <Interface><primitive>`.
// This should fail with the same "primitive type cannot be used as generic
// type argument" error as ArrayList<int> in a regular type position.

interface Predicate<T> {
    function test(T value): bool;
}

class PositivePredicate implements Predicate<int> {
    public function test(int value): bool {
        return value > 0;
    }
}

function main(): void {
    PositivePredicate p = new PositivePredicate();
    print(p.test(5));
}

main();
