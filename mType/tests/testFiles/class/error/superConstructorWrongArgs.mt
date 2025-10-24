// ERROR: Super constructor with wrong arguments

class Parent {
    int value;
    string name;

    public function Parent(int v, string n) {
        value = v;
        name = n;
    }
}

class Child extends Parent {
    public function Child() {
        // ERROR: Wrong argument types (string, int instead of int, string)
        super("wrong", 42);
    }
}

function main(): void {
    Child child = new Child();
    print("This should not execute");
}

main();
