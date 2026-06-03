// MYT-378 soundness: an Animal[16] holding a Dog (subtype, stored via the
// heterogeneous fallback) must reject narrowing a retrieved element to a
// sibling Cat. The fallback must not erase the element's concrete runtime
// class.
//
// Pinned message: "Cannot cast Dog to Cat".
print("Testing wrong-sibling cast from base-typed array");

class Animal {
    public function makeSound(): string {
        return "Generic";
    }
}

class Dog extends Animal {
    public function makeSound(): string {
        return "Woof";
    }
}

class Cat extends Animal {
    public function makeSound(): string {
        return "Meow";
    }
}

Animal[] zoo = new Animal[16];
zoo[0] = new Dog();

// Retrieved element is a Dog, not a Cat — must throw at runtime.
Cat c = (Cat)zoo[0];
print(c.makeSound());
