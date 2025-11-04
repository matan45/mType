import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Covariant return types with generics
class Producer<T> {
    T value;

    public constructor(T v) {
        value = v;
    }

    public function produce(): T {
        return value;
    }
}

class BaseFactory {
    public function create(): Producer<String> {
        return new Producer<String>(new String("Base"));
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
    String result = producer.produce();
    print(result.toString());
}

main();
