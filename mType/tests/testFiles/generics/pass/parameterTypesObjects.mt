// Test object/class type parameters
class Person {
    string name;
    int age;

    constructor(string n, int a) {
        name = n;
        age = a;
    }

    public function getName(): string {
        return name;
    }
}

function testObjectParam(Person p): string {
    return p.getName();
}

function testMultipleObjects(Person p1, Person p2): void {
    print(p1.getName());
    print(p2.getName());
}

// Call functions
Person person1 = new Person("Alice", 30);
Person person2 = new Person("Bob", 25);

print(testObjectParam(person1));
testMultipleObjects(person1, person2);
