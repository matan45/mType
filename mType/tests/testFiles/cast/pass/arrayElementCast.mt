// Test: cast an element accessed from an Animal[] to its concrete Dog subtype
class Animal {
    public string name;
    public constructor(string n) { this.name = n; }
}

class Dog extends Animal {
    public constructor(string n) : super(n) {}
    public function bark(): void { print("Woof from " + this.name); }
}

Animal[] animals = new Animal[2];
animals[0] = new Dog("Rex");
animals[1] = new Dog("Buddy");

Dog d = (Dog)animals[0];
d.bark();
print(d.name);

// Expected output:
// Woof from Rex
// Rex
