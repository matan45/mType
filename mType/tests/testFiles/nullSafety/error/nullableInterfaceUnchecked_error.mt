// Test: calling an interface method on a nullable interface receiver
// without a null check must fail at compile time.

interface Speakable {
    function speak(): string;
}

class Dog implements Speakable {
    public function speak(): string {
        return "woof";
    }
}

Speakable? maybe = new Dog();
print(maybe.speak());
