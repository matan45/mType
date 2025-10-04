// Test array literals with objects
print("Testing array literals with objects");

class Person {
    public string name;
    public int age;

    public constructor(string n, int a) {
        name = n;
        age = a;
    }

    public function toString(): string {
        return name + " (" + age + ")";
    }

    public function getName(): string {
        return name;
    }

    public function getAge(): int {
        return age;
    }
}

// Object arrays using array literals
Person[] people = [
    new Person("Alice", 25),
    new Person("Bob", 30),
    new Person("Charlie", 35)
];

print("People array length: " + people.length);
for (int i = 0; i < people.length; i++) {
    print("people[" + i + "] = " + people[i].toString());
}

// Access individual object properties
print("First person's name: " + people[0].getName());
print("Second person's age: " + people[1].getAge());

// Mixed object expressions
Person[] mixedPeople = [
    new Person("David", 28),
    people[0],  // Reference to existing object
    new Person("Eve", 32)
];

print("Mixed people array length: " + mixedPeople.length);
for (int i = 0; i < mixedPeople.length; i++) {
    print("mixedPeople[" + i + "] = " + mixedPeople[i].toString());
}

print("Array literals with objects test completed");