// Test: Factory pattern implementation
// Expected: Pass - demonstrates factory design pattern

class Animal {
    protected string name;
    protected string sound;

    public constructor(string name, string sound) {
        this.name = name;
        this.sound = sound;
    }

    public void makeSound() {
        print(this.name + " says: " + this.sound);
    }

    public string getName() {
        return this.name;
    }
}

class Dog extends Animal {
    public constructor(string name) : super(name, "Woof") {
        print("Created dog: " + name);
    }

    public void fetch() {
        print(this.name + " is fetching");
    }
}

class Cat extends Animal {
    public constructor(string name) : super(name, "Meow") {
        print("Created cat: " + name);
    }

    public void climb() {
        print(this.name + " is climbing");
    }
}

class Bird extends Animal {
    public constructor(string name) : super(name, "Tweet") {
        print("Created bird: " + name);
    }

    public void fly() {
        print(this.name + " is flying");
    }
}

class AnimalFactory {
    public static Animal createAnimal(string type, string name) {
        print("Factory creating: " + type);

        if (type == "dog") {
            return new Dog(name);
        } else if (type == "cat") {
            return new Cat(name);
        } else if (type == "bird") {
            return new Bird(name);
        } else {
            print("Unknown type, creating generic animal");
            return new Animal(name, "...");
        }
    }

    public static Animal[] createMultiple(string type, int count) {
        print("Factory creating " + count + " " + type + "s");
        Animal[] animals = new Animal[count];
        int i = 0;
        while (i < count) {
            string name = type + i;
            animals[i] = AnimalFactory.createAnimal(type, name);
            i = i + 1;
        }
        return animals;
    }
}

// Test factory pattern
print("Test 1: Create individual animals");
Animal dog = AnimalFactory.createAnimal("dog", "Buddy");
dog.makeSound();

Animal cat = AnimalFactory.createAnimal("cat", "Whiskers");
cat.makeSound();

Animal bird = AnimalFactory.createAnimal("bird", "Tweety");
bird.makeSound();

print("\nTest 2: Type-specific behavior");
if (dog instanceof Dog) {
    Dog d = (Dog)dog;
    d.fetch();
}

print("\nTest 3: Create multiple animals");
Animal[] dogs = AnimalFactory.createMultiple("dog", 3);
int i = 0;
while (i < dogs.length) {
    dogs[i].makeSound();
    i = i + 1;
}

print("\nTest 4: Unknown type");
Animal unknown = AnimalFactory.createAnimal("elephant", "Dumbo");
unknown.makeSound();
