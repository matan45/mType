// Test: upcast HashMap<K, Subtype> to HashMap<K, Supertype>; get values and
// invoke polymorphic methods through the upcast reference.
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/collections/HashMap.mt";

class Animal {
    public function speak(): void { print("Animal"); }
}

class Dog extends Animal {
    public function speak(): void { print("Woof"); }
}

HashMap<Int, Dog> dogs = new HashMap<Int, Dog>();
dogs.put(new Int(1), new Dog());
dogs.put(new Int(2), new Dog());

HashMap<Int, Animal> animals = (HashMap<Int, Animal>)dogs;
animals.get(new Int(1)).speak();
animals.get(new Int(2)).speak();
print(animals.size());

// Expected output:
// Woof
// Woof
// 2
