// Test: a nullable interface-typed receiver narrows via null check and
// then dispatches the interface method.

interface Speakable {
    function speak(): string;
}

class Dog implements Speakable {
    public string name;
    constructor(string n) {
        this.name = n;
    }
    public function speak(): string {
        return this.name + " says woof";
    }
}

Speakable? maybe = new Dog("Rex");
if (maybe != null) {
    print(maybe.speak());
}

Speakable? gone = null;
if (gone == null) {
    print("gone is null");
} else {
    print("should not reach here: " + gone.speak());
}

print("Nullable interface receiver tests passed!");
