// Test: calling a method on a nullable superclass-typed receiver without
// a null check must fail at compile time.

class Animal {
    public function speak(): string {
        return "sound";
    }
}

class Dog extends Animal {
    public function speak(): string {
        return "woof";
    }
}

Animal? a = new Dog();
print(a.speak());
