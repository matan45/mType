// Test generic instantiation cache collision
// Same type signature in different contexts should be handled correctly

class Container<T> {
    T value;
}

function createInt(): void {
    Container<int> container1 = new Container<int>();
}

function createIntAgain(): void {
    Container<int> container2 = new Container<int>();
}

class Wrapper {
    public function process(): void {
        Container<int> container3 = new Container<int>();
    }
}

function main(): void {
    // Multiple instantiations of Container<int>
    // Should not cause cache collisions
    createInt();
    createIntAgain();

    Wrapper wrapper = new Wrapper();
    wrapper.process();

    // Rapid creation
    Container<int>[] array = new Container<int>[100];

    print("Cache collision test completed");
}

main();
