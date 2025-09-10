// Set with objects test

class Person {
    string name;
    int age;
    
    constructor(string n, int a) {
        name = n;
        age = a;
    }
    
    function getName(): string {
        return name;
    }
    
    function getAge(): int {
        return age;
    }
}

// Create set of objects
Set<Person> people = new Set<Person>();

// Create different objects
Person alice = new Person("Alice", 25);
Person bob = new Person("Bob", 30);
Person aliceClone = new Person("Alice", 25); // Same data, different instance

// Add objects
print(people.add(alice));      // true
print(people.add(bob));        // true
print(people.add(aliceClone)); // true (different instance)
print(people.add(alice));      // false (same instance)

print(people.size()); // 3

// Test contains
print(people.contains(alice));      // true
print(people.contains(bob));        // true
print(people.contains(aliceClone)); // true

// Test remove
print(people.remove(bob));    // true
print(people.size());         // 2
print(people.contains(bob));  // false

print("Set objects test passed");