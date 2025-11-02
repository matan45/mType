// Class + Generics Test 1: Deep generic inheritance
@Script

class Base<T> {
    field baseValue: T;

    constructor(val: T) {
        this.baseValue = val;
    }

    fun getBase(): T {
        return this.baseValue;
    }

    fun describe(): String {
        return "Base";
    }
}

class Middle<T, U> extends Base<T> {
    field middleValue: U;

    constructor(baseVal: T, midVal: U) {
        super(baseVal);
        this.middleValue = midVal;
    }

    fun getMiddle(): U {
        return this.middleValue;
    }

    fun describe(): String {
        return "Middle";
    }
}

class Derived<T, U, V> extends Middle<T, U> {
    field derivedValue: V;

    constructor(baseVal: T, midVal: U, derVal: V) {
        super(baseVal, midVal);
        this.derivedValue = derVal;
    }

    fun getDerived(): V {
        return this.derivedValue;
    }

    fun describe(): String {
        return "Derived";
    }

    fun showAll(): Void {
        print(this.getBase());
        print(this.getMiddle());
        print(this.getDerived());
    }
}

print("Creating inheritance chain:");
let obj: Derived<Int, String, Bool> = Derived<Int, String, Bool>(42, "test", true);

print("Type description:");
print(obj.describe());

print("All values:");
obj.showAll();

print("Individual access:");
print(obj.getBase());
print(obj.getMiddle());
print(obj.getDerived());

print("Creating another chain:");
let obj2: Derived<String, Float, Int> = Derived<String, Float, Int>("hello", 3.14, 100);
print(obj2.describe());
obj2.showAll();

print("Base class reference:");
let baseRef: Base<Int> = Derived<Int, String, Bool>(99, "ref", false);
print(baseRef.describe());
print(baseRef.getBase());
