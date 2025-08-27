class Person {
    string name;
    int age;
    
    constructor(string n, int a) {
        name = n;
        age = a;
    }
}

class Company {
    string name;
    Person owner;
    
    constructor(string n, Person o) {
        name = n;
        owner = o;
    }
}

function processPerson(Person p): string {
    return p.name + " is " + toString(p.age) + " years old";
}

function processCompany(Company c, Person employee): string {
    return c.name + " owned by " + c.owner.name + ", employee: " + employee.name;
}

// Test correct calls
Person john = new Person("John", 30);
Person jane = new Person("Jane", 28);
Company corp = new Company("TechCorp", john);

string info1 = processPerson(john);
string info2 = processCompany(corp, jane);

print(info1);
print(info2);