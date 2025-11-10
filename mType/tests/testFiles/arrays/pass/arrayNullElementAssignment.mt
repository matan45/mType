// Test null assignment in object arrays
print("Testing null element assignment in object arrays");

class Person {
    string name;

    constructor(string n) {
        name = n;
    }

    public function toString(): string {
        return "Person(" + name + ")";
    }
}

// Create array of Person objects
Person[] people = new Person[4];
print("Created Person array with length: " + people.length);

// Assign some Person objects
people[0] = new Person("Alice");
people[1] = null;
people[2] = new Person("Bob");
people[3] = null;

// Access and check for null
for (int i = 0; i < people.length; i++) {
    if (people[i] == null) {
        print("people[" + i + "] = null");
    } else {
        print("people[" + i + "] = " + people[i].toString());
    }
}

// Reassign null to non-null position
people[0] = null;
print("After setting people[0] to null:");
if (people[0] == null) {
    print("people[0] is now null");
}

// Reassign non-null to null position
people[1] = new Person("Charlie");
print("After setting people[1] to Charlie:");
print("people[1] = " + people[1].toString());

print("Null element assignment test completed");
