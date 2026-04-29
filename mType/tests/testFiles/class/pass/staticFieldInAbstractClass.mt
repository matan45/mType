// Abstract class with a public static counter; constructor of the abstract
// base increments the counter so each subclass instantiation contributes.

abstract class Base {
    public static int count = 0;

    public constructor() {
        count = count + 1;
    }

    abstract function name(): string;
}

class Alpha extends Base {
    public constructor(): super() {}
    public function name(): string {
        return "Alpha";
    }
}

class Beta extends Base {
    public constructor(): super() {}
    public function name(): string {
        return "Beta";
    }
}

Alpha a1 = new Alpha();
Alpha a2 = new Alpha();
Beta b1 = new Beta();
Beta b2 = new Beta();

print(a1.name());
print(a2.name());
print(b1.name());
print(b2.name());
print("count: " + Base::count);
