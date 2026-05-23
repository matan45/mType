// MYT-360: primitive type rejected in `class Foo extends Parent<primitive>`.

class Container<T> {
    protected T value;

    constructor(T v) {
        this.value = v;
    }
}

class FloatContainer extends Container<float> {
    constructor(float v) {
        super(v);
    }
}

function main(): void {
    FloatContainer c = new FloatContainer(1.5);
    print("done");
}

main();
