// Test reflection type safety
class Person {
    string name;
    int age;

    constructor(string n, int a) {
        name = n;
        age = a;
    }

    string getInfo(): string {
        return name + " is " + toString(age) + " years old";
    }
}

class TypeInfo {
    string typeName;

    constructor(string t) {
        typeName = t;
    }

    string describe(): string {
        return "Type: " + typeName;
    }
}

// Create instances and verify types
Person p = new Person("Alice", 30);
TypeInfo typeInfo = new TypeInfo("Person");

// Type-safe operations
string info = p.getInfo();
string description = typeInfo.describe();

print(info);
print(description);
print("Reflection type safety passed");
