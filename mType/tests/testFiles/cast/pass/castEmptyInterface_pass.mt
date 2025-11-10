// Test casting to/from empty interface
// Empty interfaces with no methods should still support casting

interface Empty {
    // Completely empty interface
}

interface AlsoEmpty extends Empty {
    // Extends empty interface but adds nothing
}

class ConcreteClass implements Empty {
    int value;

    constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

class AnotherClass implements AlsoEmpty {
    string text;

    constructor(string t) {
        this.text = t;
    }

    public function getText(): string {
        return this.text;
    }
}

// Test casting to empty interface
ConcreteClass obj1 = new ConcreteClass(42);
Empty e1 = (Empty)obj1;
print("Cast to empty interface successful");

// Cast back
ConcreteClass back1 = (ConcreteClass)e1;
print(back1.getValue());

// Test with inheritance chain
AnotherClass obj2 = new AnotherClass("hello");
AlsoEmpty e2 = (AlsoEmpty)obj2;
Empty e3 = (Empty)e2;  // Cast to parent empty interface
print("Cast through empty interface hierarchy successful");

// Cast back through chain
AlsoEmpty temp = (AlsoEmpty)e3;
AnotherClass back2 = (AnotherClass)temp;
print(back2.getText());

// Multiple empty interface implementations
class MultiEmpty implements Empty, AlsoEmpty {
    float number;

    constructor(float n) {
        this.number = n;
    }

    public function getNumber(): float {
        return this.number;
    }
}

MultiEmpty obj3 = new MultiEmpty(3.14);
Empty empty1 = (Empty)obj3;
AlsoEmpty empty2 = (AlsoEmpty)obj3;
print("Multiple empty interface cast successful");

MultiEmpty back3 = (MultiEmpty)empty1;
print(back3.getNumber());

print("Empty interface casting test passed");
