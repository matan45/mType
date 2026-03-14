// Test string interpolation with method calls
class Person {
    private string name;

    constructor(string name) {
        this.name = name;
    }

    public function getName(): string {
        return this.name;
    }
}

Person p = new Person("Alice");
string result = $"Name: {p.getName()}";
print(result);
