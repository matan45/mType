// Test: Null cast in ternary operator
class Animal {
    string name;

    Animal(string n) {
        this.name = n;
    }
}

class Dog extends Animal {
    int age;

    Dog(string n, int a) : Animal(n) {
        this.age = a;
    }
}

class Cat extends Animal {
    bool indoor;

    Cat(string n, bool i) : Animal(n) {
        this.indoor = i;
    }
}

// Simple ternary with nullable cast
Animal? a1 = new Dog("Rex", 5);
Animal? a2 = null;

Dog? dog1 = a1 != null ? (Dog?)(Animal)a1 : null;
Dog? dog2 = a2 != null ? (Dog?)(Animal)a2 : null;

print(dog1 != null);
print(dog2 == null);

// Nested ternary with casts
bool hasAnimal = true;
bool isDog = true;

Animal? animal = hasAnimal ? (isDog ? (Animal?)new Dog("Buddy", 3) : (Animal?)new Cat("Whiskers", true)) : null;
print(animal != null);

Dog? resultDog = (animal != null && isDog) ? (Dog?)(Animal)animal : null;
print(resultDog != null);
print(resultDog != null ? resultDog.age : 0);

// Ternary with multiple nullable casts
Animal? nullAnimal = null;
string result = nullAnimal != null ? ((Animal)nullAnimal).name : "No animal";
print(result);

// Expected output:
// true
// true
// true
// true
// 3
// No animal
