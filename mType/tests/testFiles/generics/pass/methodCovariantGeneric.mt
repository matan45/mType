import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Covariant return types with generics
class Producer<T> {
    T value;

    public function Producer(T v) {
        value = v;
    }

    public function produce(): T {
        return value;
    }
}

class BaseFactory {
    public function create(): Producer<Object> {
        return new Producer<Object>(new Object());
    }
}

class StringFactory extends BaseFactory {
    public function create(): Producer<String> {
        return new Producer<String>(new String("Created"));
    }
}

function main(): void {
    StringFactory factory = new StringFactory();
    Producer<String> producer = factory.create();
    print(producer.produce());
}

main();
