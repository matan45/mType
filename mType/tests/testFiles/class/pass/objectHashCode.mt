// Test: hashCode() implementation
// Expected: Pass - demonstrates hashCode override

class Person {
    private string name;
    private int age;

    public constructor(string name, int age) {
        this.name = name;
        this.age = age;
    }

    public function hashCode(): int {
        // Simple hash combining name length and age
        int hash = 0;
        if (this.name != null) {
            hash = strLength(this.name) * 31;
        }
        hash = hash + this.age;
        print("hashCode() called for " + this.name + ": " + hash);
        return hash;
    }

    public function equals(Person other): bool {
        if (other == null) {
            return false;
        }
        Person p = other;
        return this.name == p.name && this.age == p.age;
    }

    public function toString(): string {
        return this.name + " (age " + this.age + ")";
    }
}

// Test hashCode implementation
print("Test 1: Same values, same hash");
Person p1 = new Person("Alice", 30);
Person p2 = new Person("Alice", 30);
int h1 = p1.hashCode();
int h2 = p2.hashCode();
print("Hash codes equal: " + (h1 == h2));
print("Objects equal: " + p1.equals(p2));

print("\nTest 2: Different values, likely different hash");
Person p3 = new Person("Bob", 25);
int h3 = p3.hashCode();
print("h1 == h3: " + (h1 == h3));

print("\nTest 3: Hash consistency");
int h1a = p1.hashCode();
int h1b = p1.hashCode();
print("Same object, same hash: " + (h1a == h1b));

print("\nTest 4: Different ages, different hashes");
Person p4 = new Person("Alice", 31);
int h4 = p4.hashCode();
print("Same name, different age:");
print("  h1: " + h1);
print("  h4: " + h4);
print("  Different: " + (h1 != h4));
