// Test: generic type arguments are invariant. `Box<Dog> isClassOf Box<Animal>`
// must return false, even though Dog extends Animal. This pins mType's
// invariant-generics stance at the isClassOf surface.

import * from "../../lib/primitives/Box.mt";

class Animal {
    public string name;
    public constructor(string name) { this.name = name; }
}

class Dog extends Animal {
    public constructor(string name) { super(name); }
}

Box<Dog> boxDog = new Box<Dog>(new Dog("rex"));

print(boxDog isClassOf Box<Dog>);    // true
print(boxDog isClassOf Box<Animal>); // false (invariant — Dog extends Animal, but Box<Dog> is NOT Box<Animal>)
print(boxDog isClassOf Box);         // true (raw fallback still allowed)
