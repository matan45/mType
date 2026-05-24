// Test: upcast ArrayList<Subtype> to ArrayList<Supertype> (generics erase at
// runtime); iterate through the upcast list and verify polymorphic dispatch.
import * from "../../lib/collections/ArrayList.mt";

class Animal {
    public function speak(): void { print("Animal"); }
}

class Dog extends Animal {
    public function speak(): void { print("Woof"); }
}

ArrayList<Dog> dogs = new ArrayList<Dog>();
dogs.add(new Dog());
dogs.add(new Dog());

ArrayList<Animal> animals = (ArrayList<Animal>)dogs;
for (int i = 0; i < animals.size(); i++) {
    animals.get(i).speak();
}

// Expected output:
// Woof
// Woof
