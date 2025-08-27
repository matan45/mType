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
    
    function isAdult(): bool {
        return age >= 18;
    }
}

Person p = new Person("John", 25);
string personName = p.getName();
int personAge = p.getAge();
bool adult = p.isAdult();

print("Object method returns test passed");