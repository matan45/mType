import * from "../../../lib/primitives/Int.mt";
import * from "../../../lib/primitives/String.mt";

// Test wildcard with lower bounds conceptually (super semantics)
// Since mType may not have explicit 'super' keyword, we test contravariance
interface Producer<T> {
    function produce(): T;
}

class IntProducer implements Producer<Int> {
    int value;

    constructor(int v) {
        value = v;
    }

    public function produce(): Int {
        return new Int(value);
    }
}

class StringProducer implements Producer<String> {
    string value;

    constructor(string v) {
        value = v;
    }

    public function produce(): String {
        return new String(value);
    }
}

// Generic consumer that can work with any producer
class Consumer<T> {
    Producer<T> producer;

    constructor(Producer<T> p) {
        producer = p;
    }

    public function consume(): T {
        return producer.produce();
    }

    public function consumeMultiple(int count): void {
        int i = 0;
        while (i < count) {
            T item = producer.produce();
            print("Consumed: " + item);
            i = i + 1;
        }
    }
}

function main(): void {
    print("Testing wildcard with lower bound semantics");

    IntProducer intProd = new IntProducer(42);
    Consumer<Int> intConsumer = new Consumer<Int>(intProd);
    print("First consumption: " + intConsumer.consume());
    intConsumer.consumeMultiple(3);

    StringProducer strProd = new StringProducer("Hello");
    Consumer<String> strConsumer = new Consumer<String>(strProd);
    print("String consumption: " + strConsumer.consume());
    strConsumer.consumeMultiple(2);

    print("Lower bound wildcard test completed");
}

main();
