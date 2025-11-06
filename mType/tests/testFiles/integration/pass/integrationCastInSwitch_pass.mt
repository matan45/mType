// Integration Test: Casting in switch statements
// Tests casting within switch cases and expressions

class Animal {
    public String name;

    public constructor(String n) {
        this.name = n;
    }

    public Int getType() {
        return 0;
    }

    public String makeSound() {
        return "Generic sound";
    }
}

class Dog extends Animal {
    public String breed;

    public constructor(String n, String b) {
        super(n);
        this.breed = b;
    }

    public Int getType() {
        return 1;
    }

    public String makeSound() {
        return "Woof!";
    }

    public String fetch() {
        return this.name + " is fetching";
    }
}

class Cat extends Animal {
    public Bool indoor;

    public constructor(String n, Bool i) {
        super(n);
        this.indoor = i;
    }

    public Int getType() {
        return 2;
    }

    public String makeSound() {
        return "Meow!";
    }

    public String climb() {
        return this.name + " is climbing";
    }
}

class Bird extends Animal {
    public Float wingspan;

    public constructor(String n, Float w) {
        super(n);
        this.wingspan = w;
    }

    public Int getType() {
        return 3;
    }

    public String makeSound() {
        return "Tweet!";
    }

    public String fly() {
        return this.name + " is flying";
    }
}

// Test 1: Switch with casting in cases
print("Test 1: Switch with casting in cases");
Animal animals[] = new Animal[4];
animals[0] = new Dog("Rex", "Labrador");
animals[1] = new Cat("Whiskers", true);
animals[2] = new Bird("Tweety", 0.5);
animals[3] = new Dog("Max", "Beagle");

for (Int i = 0; i < 4; i = i + 1) {
    switch (animals[i].getType()) {
        case 1:
            Dog d = (Dog)animals[i];
            print("Dog: " + d.name + " (" + d.breed + ") - " + d.makeSound());
            break;
        case 2:
            Cat c = (Cat)animals[i];
            print("Cat: " + c.name + " (indoor: " + c.indoor + ") - " + c.makeSound());
            break;
        case 3:
            Bird b = (Bird)animals[i];
            print("Bird: " + b.name + " (wingspan: " + b.wingspan + ") - " + b.makeSound());
            break;
        default:
            print("Unknown animal type");
            break;
    }
}

// Test 2: Switch on cast result
print("Test 2: Switch on cast result");
Animal a1 = new Dog("Buddy", "Golden");
Int typeCode = ((Dog)a1).getType();

switch (typeCode) {
    case 1:
        print("Processing dog type");
        print(((Dog)a1).fetch());
        break;
    case 2:
        print("Processing cat type");
        break;
    default:
        print("Other type");
        break;
}

// Test 3: Nested switch with multiple casts
print("Test 3: Nested switch with casting");
Animal testAnimal = new Cat("Fluffy", false);
Int category = testAnimal.getType();

switch (category) {
    case 1:
        Dog dog = (Dog)testAnimal;
        switch (dog.breed.length()) {
            case 6:
                print("6-letter breed");
                break;
            case 8:
                print("8-letter breed");
                break;
            default:
                print("Other breed length");
                break;
        }
        break;
    case 2:
        Cat cat = (Cat)testAnimal;
        switch (cat.indoor) {
            case true:
                print("Indoor cat: " + cat.name);
                break;
            case false:
                print("Outdoor cat: " + cat.name);
                break;
            default:
                print("Unknown cat location");
                break;
        }
        break;
    case 3:
        print("Bird case");
        break;
    default:
        print("Unknown animal");
        break;
}

// Test 4: Switch with casting and method calls
print("Test 4: Switch with cast method calls");
Animal polymorphicArray[] = new Animal[3];
polymorphicArray[0] = new Dog("Spot", "Dalmatian");
polymorphicArray[1] = new Cat("Shadow", true);
polymorphicArray[2] = new Bird("Eagle", 2.5);

for (Int idx = 0; idx < 3; idx = idx + 1) {
    Int type = polymorphicArray[idx].getType();
    switch (type) {
        case 1:
            print(((Dog)polymorphicArray[idx]).fetch());
            break;
        case 2:
            print(((Cat)polymorphicArray[idx]).climb());
            break;
        case 3:
            print(((Bird)polymorphicArray[idx]).fly());
            break;
        default:
            print("No special action");
            break;
    }
}

// Test 5: Switch controlling cast behavior
print("Test 5: Switch controlling cast flow");
Int animalId = 2;
Animal genericAnimal = new Cat("Mittens", true);

switch (animalId) {
    case 1:
        if (genericAnimal isClassOf Dog) {
            print("Expected dog, got dog");
        } else {
            print("Expected dog, got something else");
        }
        break;
    case 2:
        if (genericAnimal isClassOf Cat) {
            Cat myCat = (Cat)genericAnimal;
            print("Expected cat: " + myCat.name + " (correct)");
        } else {
            print("Expected cat, got something else");
        }
        break;
    default:
        print("Unknown ID");
        break;
}

// Test 6: Multiple switches with same cast
print("Test 6: Multiple switches with same object");
Animal multiSwitch = new Dog("Charlie", "Poodle");
Dog castedDog = (Dog)multiSwitch;

switch (castedDog.name.length()) {
    case 7:
        print("Name length is 7");
        break;
    default:
        print("Name length is not 7");
        break;
}

switch (castedDog.breed.length()) {
    case 6:
        print("Breed length is 6");
        break;
    default:
        print("Breed length is not 6");
        break;
}

print("All switch casting tests completed");
