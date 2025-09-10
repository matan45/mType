// Test two-phase comparison in Set (hash + equals)
class Person {
    string name;
    int age;
    
    constructor(string n, int a){
		name = n;
		age = a;
    }
    
    function getName():string {
        return name;
    }
    
    function getAge(): int {
        return age;
    }
}

function testTwoPhaseComparison(): void {
    print("=== Testing Two-Phase Comparison in Set ===");
    
    Set<Person> people = new Set<Person>();
    
    // Create objects with same content
    Person alice1 = new Person("Alice", 25);
    Person alice2 = new Person("Alice", 25);  // Same content, different instance
    Person bob = new Person("Bob", 30);       // Different content
    Person alice3 = new Person("Alice", 26);  // Same name, different age
    
    print("Adding alice1: " + people.add(alice1));      // Should be true
    print("Size after alice1: " + people.size());
    
    print("Adding alice2 (same content): " + people.add(alice2));  // Should be false (duplicate)
    print("Size after alice2: " + people.size());
    
    print("Adding bob: " + people.add(bob));            // Should be true
    print("Size after bob: " + people.size());
    
    print("Adding alice3 (different age): " + people.add(alice3));  // Should be true
    print("Final size: " + people.size());
    
    print("\n=== Testing Contains ===");
    Person alice4 = new Person("Alice", 25);  // Same content as alice1
    print("Contains alice4 (same content as alice1): " + people.contains(alice4));  // Should be true
    
    Person charlie = new Person("Charlie", 35);
    print("Contains charlie: " + people.contains(charlie));  // Should be false
    
    print("\n=== Testing Remove ===");
    Person alice5 = new Person("Alice", 25);  // Same content as alice1
    print("Removing alice5 (same content as alice1): " + people.remove(alice5));  // Should be true
    print("Size after removal: " + people.size());
    
    print("Contains alice1 after removal: " + people.contains(alice1));  // Should be false
}

testTwoPhaseComparison();