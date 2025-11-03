
// Test casting in constructor parameter
class Wrapper {
    string value;

    constructor(string v) {
        this.value = v;
    }

    public function getValue(): string {
        return this.value;
    }
}

class Base {
    public function getDescription(): string {
        return "Base";
    }
}

class Derived extends Base {
    public function getDescription(): string {
        return "Derived";
    }
}

class Container {
    Wrapper wrapped;

    constructor(Base obj) {
        // Cast and use in constructor
        this.wrapped = new Wrapper(obj.getDescription());
    }

    public function getWrappedValue(): string {
        return this.wrapped.getValue();
    }
}

function createContainer(Derived d): Container {
    // Cast in parameter passing
    return new Container(d);
}

function main(): void {
    Derived derived = new Derived();
    Container container = createContainer(derived);

    print(container.getWrappedValue());

    // Direct casting in constructor call
    Container container2 = new Container(derived);
    print(container2.getWrappedValue());

    // Multiple level casting
    Base base = derived;
    Container container3 = new Container(base);
    print(container3.getWrappedValue());
}

main();
