// MYT-378 soundness: widening a Dog into an Object[16] is allowed, but
// narrowing the retrieved element to a sibling type (Cat) must still throw at
// runtime. The heterogeneous fallback preserves the real runtime class, so the
// cast check fires correctly.
//
// The cast must throw (ERROR_EXPECTED) — confirming the fallback did not
// silently coerce or lose the element's runtime identity. The uncaught cast
// surfaces only as "User exception thrown: Exception", so the exact message
// is not asserted here.
print("Testing wrong-sibling cast after Object widening");

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

Object[] beans = new Object[16];
beans[0] = new Dog();

// Dog is not a Cat — must throw at runtime.
Cat c = (Cat)beans[0];
print(c.makeSound());
