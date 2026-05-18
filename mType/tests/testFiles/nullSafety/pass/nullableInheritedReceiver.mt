// Test: a nullable receiver typed at a superclass narrows and dispatches
// virtually through the subclass override.

class Animal {
    public string name;
    constructor(string n) {
        this.name = n;
    }
    public function speak(): string {
        return this.name + " makes a sound";
    }
}

class Dog extends Animal {
    constructor(string n) : super(n) {}
    public function speak(): string {
        return this.name + " woofs";
    }
}

Animal? maybeDog = new Dog("Rex");
if (maybeDog != null) {
    print(maybeDog.speak());
}

Animal? plain = new Animal("Generic");
if (plain != null) {
    print(plain.speak());
}

Animal? empty = null;
if (empty == null) {
    print("empty is null");
}

print("Nullable inherited receiver tests passed!");
