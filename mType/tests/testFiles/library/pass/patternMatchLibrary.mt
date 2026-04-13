class Cat {
    public string name;
    public constructor(string name) {
        this.name = name;
    }
}

class Dog {
    public string name;
    public constructor(string name) {
        this.name = name;
    }
}

function describe(Object pet): string {
    if (pet isClassOf Cat) {
        Cat c = (Cat)pet;
        return "Cat: " + c.name;
    }
    if (pet isClassOf Dog) {
        Dog d = (Dog)pet;
        return "Dog: " + d.name;
    }
    return "Unknown pet";
}

print(describe(new Cat("Whiskers")));
print(describe(new Dog("Buddy")));
