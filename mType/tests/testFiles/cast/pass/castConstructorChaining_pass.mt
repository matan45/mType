@Script
// Test casting in constructor parameter
class Wrapper {
    let value: String;

    fn init(v: String) {
        this.value = v;
    }

    fn getValue(): String {
        return this.value;
    }
}

class Base {
    fn toString(): String {
        return "Base";
    }
}

class Derived extends Base {
    fn toString(): String {
        return "Derived";
    }
}

class Container {
    let wrapped: Wrapper;

    fn init(obj: Base) {
        // Cast and use in constructor
        this.wrapped = new Wrapper(obj.toString());
    }

    fn getWrappedValue(): String {
        return this.wrapped.getValue();
    }
}

fn createContainer(d: Derived): Container {
    // Cast in parameter passing
    return new Container(d as Base);
}

fn main() {
    let derived: Derived = new Derived();
    let container: Container = createContainer(derived);

    print(container.getWrappedValue());

    // Direct casting in constructor call
    let container2: Container = new Container(derived as Base);
    print(container2.getWrappedValue());

    // Multiple level casting
    let base: Base = derived as Base;
    let container3: Container = new Container(base);
    print(container3.getWrappedValue());
}
