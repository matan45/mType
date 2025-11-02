import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// instanceof checks with generic types
class Container<T> {
    T data;

    public function setData(T d): void {
        data = d;
    }

    public function hasData(): Bool {
        return data != null;
    }
}

class Base {
    String id;

    public function Base(String i) {
        id = i;
    }
}

class Derived extends Base {
    public function Derived(String i) {
        super(i);
    }
}

function checkContainer(Container<Base> container): void {
    if (container.hasData()) {
        print("Container has data");
    } else {
        print("Container is empty");
    }
}

function main(): void {
    Container<Derived> derivedContainer = new Container<Derived>();
    derivedContainer.setData(new Derived(new String("D1")));

    // Check runtime type despite erasure
    checkContainer(derivedContainer);
}

main();
