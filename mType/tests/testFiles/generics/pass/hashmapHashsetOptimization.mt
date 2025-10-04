// Test HashMap and HashSet contains() methods for hash-based optimization
import "../../lib/collections/HashMap.mt";
import "../../lib/collections/HashSet.mt";
import "../../lib/primitives/String.mt";
import "../../lib/primitives/Int.mt";

class Person {
    String name;
    Int age;

    public constructor(String name, Int age) {
        this.name = name;
        this.age = age;
    }

    public function getName(): String {
        return this.name;
    }

    public function getAge(): Int {
        return this.age;
    }

    public function equals(Person other): bool {
        if (other == null) {
            return false;
        }
        return this.name.equals(other.name) && this.age.equals(other.age);
    }

    public function hashCode(): int {
        int nameHash = this.name.hashCode();
        int ageHash = this.age.hashCode();
        return nameHash + ageHash * 31; // Combine hash codes
    }
}

print("Testing HashMap containsKey() optimization:");

// Test HashMap with String keys
HashMap<String, Int> scores = new HashMap<String, Int>();
scores.put(new String("Alice"), new Int(85));
scores.put(new String("Bob"), new Int(92));
scores.put(new String("Charlie"), new Int(78));
scores.put(new String("David"), new Int(81));
scores.put(new String("Eve"), new Int(94));

print("HashMap size: " + scores.size());
print("Contains Alice: " + scores.containsKey(new String("Alice")));
print("Contains Bob: " + scores.containsKey(new String("Bob")));
print("Contains Frank: " + scores.containsKey(new String("Frank")));

// Test HashMap with object keys
HashMap<Person, String> roles = new HashMap<Person, String>();
Person alice = new Person(new String("Alice"), new Int(25));
Person bob = new Person(new String("Bob"), new Int(30));
Person charlie = new Person(new String("Charlie"), new Int(28));

roles.put(alice, new String("Developer"));
roles.put(bob, new String("Manager"));
roles.put(charlie, new String("Designer"));

print("\nHashMap with object keys:");
print("Roles map size: " + roles.size());
print("Contains Alice: " + roles.containsKey(alice));
print("Contains Charlie: " + roles.containsKey(charlie));

Person newPerson = new Person(new String("Diana"), new Int(32));
print("Contains Diana: " + roles.containsKey(newPerson));

print("\n==================================================");
print("Testing HashSet contains() optimization:");

// Test HashSet with Strings
HashSet<String> cities = new HashSet<String>();
cities.add(new String("New York"));
cities.add(new String("London"));
cities.add(new String("Tokyo"));
cities.add(new String("Paris"));
cities.add(new String("Sydney"));

print("HashSet size: " + cities.size());
print("Contains New York: " + cities.contains(new String("New York")));
print("Contains London: " + cities.contains(new String("London")));
print("Contains Berlin: " + cities.contains(new String("Berlin")));

// Test HashSet with objects
HashSet<Person> people = new HashSet<Person>();
people.add(alice);
people.add(bob);
people.add(charlie);

print("\nHashSet with objects:");
print("People set size: " + people.size());
print("Contains Alice: " + people.contains(alice));
print("Contains Bob: " + people.contains(bob));
print("Contains Diana: " + people.contains(newPerson));

// Test content-based equality
Person alice2 = new Person(new String("Alice"), new Int(25)); // Same content as alice
print("Contains Alice2 (same content): " + people.contains(alice2));

print("\n==================================================");
print("Testing large dataset performance:");

// Test with larger dataset to verify hash performance
HashMap<Int, String> largeHashMap = new HashMap<Int, String>();
HashSet<Int> largeHashSet = new HashSet<Int>();

for (int i = 0; i < 100; i++) {
    largeHashMap.put(new Int(i), new String("Value" + i));
    largeHashSet.add(new Int(i));
}

print("Large HashMap size: " + largeHashMap.size());
print("Large HashSet size: " + largeHashSet.size());

// Test contains performance
print("HashMap contains key 50: " + largeHashMap.containsKey(new Int(50)));
print("HashMap contains key 150: " + largeHashMap.containsKey(new Int(150)));
print("HashSet contains 75: " + largeHashSet.contains(new Int(75)));
print("HashSet contains 200: " + largeHashSet.contains(new Int(200)));

// Test collision handling
print("\nTesting hash collision handling:");
HashMap<Int, String> collisionTest = new HashMap<Int, String>();

// Add items that might cause collisions
for (int i = 0; i < 20; i++) {
    collisionTest.put(new Int(i), new String("Item" + i));
}

print("Collision test map size: " + collisionTest.size());
print("Contains key 5: " + collisionTest.containsKey(new Int(5)));
print("Contains key 15: " + collisionTest.containsKey(new Int(15)));
print("Contains key 25: " + collisionTest.containsKey(new Int(25)));

print("\nHashMap and HashSet tests completed successfully!");