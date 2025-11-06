// Class + Generics Test 1: Deep generic inheritance
@Script

class Base<T> {
    T baseValue;

    constructor(T val) {
        this.baseValue = val;
    }

    public function getBase(): T {
        return this.baseValue;
    }

    public function describe(): String {
        return "Base";
    }
}

class Middle<T, U> extends Base<T> {
    U middleValue;

    constructor(T baseVal, U midVal) {
        super(baseVal);
        this.middleValue = midVal;
    }

    public function getMiddle(): U {
        return this.middleValue;
    }

    public function describe(): String {
        return "Middle";
    }
}

class Derived<T, U, V> extends Middle<T, U> {
    V derivedValue;

    constructor(T baseVal, U midVal, V derVal) {
        super(baseVal, midVal);
        this.derivedValue = derVal;
    }

    public function getDerived(): V {
        return this.derivedValue;
    }

    public function describe(): String {
        return "Derived";
    }

    public function showAll(): void {
        print(this.getBase());
        print(this.getMiddle());
        print(this.getDerived());
    }
}

print("Creating inheritance chain:");
Derived<Int, String, Bool> obj = Derived<Int, String, Bool>(42, "test", true);

print("Type description:");
print(obj.describe());

print("All values:");
obj.showAll();

print("Individual access:");
print(obj.getBase());
print(obj.getMiddle());
print(obj.getDerived());

print("Creating another chain:");
Derived<String, Float, Int> obj2 = Derived<String, Float, Int>("hello", 3.14, 100);
print(obj2.describe());
obj2.showAll();

print("Base class reference:");
Base<Int> baseRef = Derived<Int, String, Bool>(99, "ref", false);
print(baseRef.describe());
print(baseRef.getBase());
