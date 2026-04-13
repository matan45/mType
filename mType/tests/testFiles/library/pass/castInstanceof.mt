class Animal {
    public string name;
    public constructor(string name) {
        this.name = name;
    }
}

class Dog extends Animal {
    public constructor(string name) : super(name) {
    }

    public function bark(): string {
        return this.name + " says Woof!";
    }
}

Animal a = new Dog("Rex");
print(a isClassOf Animal);
print(a isClassOf Dog);

Dog d = (Dog)a;
print(d.bark());
