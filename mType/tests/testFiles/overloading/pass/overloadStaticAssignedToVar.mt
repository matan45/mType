// Test: An overloaded static call's result is assigned to a typed local
// variable. The variable-declaration type-check must accept the resolved
// return type from the matched overload.

class Item {
    public int n;
    public constructor(int x) { this.n = x; }
}

class Factory {
    public static function build(int seed): Item {
        return new Item(seed);
    }
    public static function build(int seed, int multiplier): Item {
        return new Item(seed * multiplier);
    }
}

Item one = Factory::build(5);
Item two = Factory::build(5, 3);
print(one.n);
print(two.n);
