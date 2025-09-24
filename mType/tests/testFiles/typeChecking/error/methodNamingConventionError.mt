// Test: Method names must start with lowercase letter
// Expected Error: Method 'GetName' must start with a lowercase letter. Functions and methods should follow camelCase convention.

class Person {
    string name;

    constructor(string n) {
        name = n;
    }

    function GetName(): string {
        return name;
    }
}

function main(): void {
    Person p = new Person("Alice");
    string result = p.GetName();
    print(result);
}
main();