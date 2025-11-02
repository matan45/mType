// Test: Exception handling during cast operations
// Expected: Pass - demonstrates exception handling with casting

import * from "../../lib/exceptions/Exception.mt";

class CastException extends Exception {
    constructor(string msg):super(msg) {
    }
}

class Animal {
    protected string name;

    public constructor(string name) {
        this.name = name;
    }

    public string getName() {
        return this.name;
    }

    public void speak() {
        print("Animal " + this.name + " makes a sound");
    }
}

class Dog extends Animal {
    private string breed;

    public constructor(string name, string breed) : super(name) {
        this.breed = breed;
    }

    public void speak() {
        print("Dog " + this.name + " barks");
    }

    public string getBreed() {
        return this.breed;
    }

    public void fetch() {
        print(this.name + " fetches the ball");
    }
}

class Cat extends Animal {
    private int lives;

    public constructor(string name, int lives) : super(name) {
        this.lives = lives;
    }

    public void speak() {
        print("Cat " + this.name + " meows");
    }

    public int getLives() {
        return this.lives;
    }
}

// Test 1: Exception thrown during cast operation
function safeCastToDog(Animal a): string {
    try {
        if (a instanceof Dog) {
            Dog d = (Dog)a;
            d.fetch();
            return "Success: Cast to Dog";
        } else {
            throw new CastException("Cannot cast to Dog - wrong type");
        }
    } catch (CastException e) {
        return "Caught: " + e.getMessage();
    }
}

// Test 2: Multiple type checks with exception handling
function processAnimal(Animal a): string {
    try {
        print("Processing: " + a.getName());
        a.speak();

        if (a instanceof Dog) {
            Dog d = (Dog)a;
            print("Breed: " + d.getBreed());
            return "Processed Dog";
        } else if (a instanceof Cat) {
            Cat c = (Cat)a;
            print("Lives: " + c.getLives());
            return "Processed Cat";
        } else {
            throw new CastException("Unknown animal type");
        }
    } catch (CastException e) {
        return "Error: " + e.getMessage();
    }
}

// Test 3: Exception in finally after successful cast
function castWithFinallyCleanup(Animal a): string {
    try {
        if (a instanceof Dog) {
            Dog d = (Dog)a;
            print("Cast successful: " + d.getName());
            return "Dog cast complete";
        } else {
            return "Not a dog";
        }
    } finally {
        print("Cleanup: Finished processing " + a.getName());
    }
}

// Test 4: Nested try-catch with casting
function nestedCastException(Animal a): string {
    try {
        try {
            if (a instanceof Cat) {
                Cat c = (Cat)a;
                int lives = c.getLives();
                if (lives < 1) {
                    throw new CastException("Cat has no lives left");
                }
                return "Cat has " + lives + " lives";
            } else {
                throw new CastException("Not a cat");
            }
        } catch (CastException e) {
            print("Inner catch: " + e.getMessage());
            throw e;
        }
    } catch (CastException e) {
        return "Outer catch: " + e.getMessage();
    }
}

// Test 5: Cast in exception handler
function castInCatchBlock(Animal a): string {
    try {
        throw new CastException("Intentional exception");
    } catch (CastException e) {
        print("In catch block: " + e.getMessage());
        if (a instanceof Dog) {
            Dog d = (Dog)a;
            return "Exception handled, cast to Dog: " + d.getBreed();
        } else {
            return "Exception handled, not a dog";
        }
    }
}

// Run all tests
print("=== Test 1: Exception during cast operation ===");
Animal dog1 = new Dog("Buddy", "Golden Retriever");
print(safeCastToDog(dog1));

Animal cat1 = new Cat("Whiskers", 9);
print(safeCastToDog(cat1));

print("\n=== Test 2: Multiple type checks with exceptions ===");
Animal dog2 = new Dog("Max", "Labrador");
print(processAnimal(dog2));

Animal cat2 = new Cat("Mittens", 7);
print(processAnimal(cat2));

print("\n=== Test 3: Cast with finally cleanup ===");
Animal dog3 = new Dog("Rex", "German Shepherd");
print(castWithFinallyCleanup(dog3));

Animal cat3 = new Cat("Shadow", 5);
print(castWithFinallyCleanup(cat3));

print("\n=== Test 4: Nested exception with casting ===");
Animal cat4 = new Cat("Luna", 3);
print(nestedCastException(cat4));

Animal dog4 = new Dog("Duke", "Beagle");
print(nestedCastException(dog4));

print("\n=== Test 5: Cast in exception handler ===");
Animal dog5 = new Dog("Charlie", "Poodle");
print(castInCatchBlock(dog5));

Animal cat5 = new Cat("Oliver", 8);
print(castInCatchBlock(cat5));

print("\nAll cast exception handling tests completed");
