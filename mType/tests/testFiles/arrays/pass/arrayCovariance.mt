// Test subtype array to supertype assignment (covariance)
print("Testing array covariance");

class Animal {
    string name;

    Animal(string n) {
        name = n;
    }

    string getName() {
        return name;
    }

    string makeSound() {
        return "Some sound";
    }
}

class Dog : Animal {
    Dog(string n) : Animal(n) {
    }

    string makeSound() {
        return "Woof!";
    }
}

class Cat : Animal {
    Cat(string n) : Animal(n) {
    }

    string makeSound() {
        return "Meow!";
    }
}

// Create Dog array
Dog[] dogs = new Dog[2];
dogs[0] = new Dog("Buddy");
dogs[1] = new Dog("Max");

print("Dog array:");
for (int i = 0; i < dogs.length; i++) {
    print("  " + dogs[i].getName() + " says " + dogs[i].makeSound());
}

// Assign to Animal array (covariance)
Animal[] animals = new Animal[3];
animals[0] = dogs[0];
animals[1] = dogs[1];
animals[2] = new Cat("Whiskers");

print("Animal array (with dogs and cat):");
for (int i = 0; i < animals.length; i++) {
    print("  " + animals[i].getName() + " says " + animals[i].makeSound());
}

print("Array covariance test completed");
