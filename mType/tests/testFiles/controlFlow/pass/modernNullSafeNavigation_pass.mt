// Modern null-safe navigation patterns using conditional checks
// Tests safe object traversal and method invocation

class Address {
    public string street;
    public string city;

    constructor(string s, string c) {
        street = s;
        city = c;
    }

    public function toString(): string {
        return street + ", " + city;
    }
}

class Person {
    public string name;
    public Address? address;

    constructor(string n, Address? a) {
        name = n;
        address = a;
    }

    public function getAddressString(): string {
        return address != null ? address.toString() : "No address";
    }
}

class Company {
    public Person? ceo;

    constructor(Person? c) {
        ceo = c;
    }

    public function getCeoName(): string {
        return ceo != null ? ceo.name : "No CEO";
    }
}

// Test 1: Simple null-safe property access
print("Test 1: Simple null-safe property access");
Person? p1 = null;
string name1 = "Unknown";
if (p1 != null) {
    name1 = p1.name;
}
print(name1);

Person p2 = new Person("Alice", null);
string name2 = p2 != null ? p2.name : "Unknown";
print(name2);

// Test 2: Nested null-safe navigation
print("Test 2: Nested null-safe navigation");
Person p3 = new Person("Bob", new Address("Main St", "NYC"));
string city1 = p3 != null ? (p3.address != null ? p3.address.city : "No city") : "No person";
print(city1);

Person p4 = new Person("Charlie", null);
string city2 = p4 != null ? (p4.address != null ? p4.address.city : "No city") : "No person";
print(city2);

Person? p5 = null;
string city3 = "No person";
if (p5 != null) {
    city3 = p5.address != null ? p5.address.city : "No city";
}
print(city3);

// Test 3: Deep null-safe navigation chain
print("Test 3: Deep null-safe navigation chain");
Company company1 = new Company(new Person("CEO1", new Address("Oak Ave", "LA")));
string ceoCity1 = company1 != null ?
                  (company1.ceo != null ?
                   (company1.ceo.address != null ?
                    company1.ceo.address.city : "No address") : "No CEO") : "No company";
print(ceoCity1);

Company company2 = new Company(new Person("CEO2", null));
string ceoCity2 = company2 != null ?
                  (company2.ceo != null ?
                   (company2.ceo.address != null ?
                    company2.ceo.address.city : "No address") : "No CEO") : "No company";
print(ceoCity2);

Company company3 = new Company(null);
string ceoCity3 = company3 != null ?
                  (company3.ceo != null ?
                   (company3.ceo.address != null ?
                    company3.ceo.address.city : "No address") : "No CEO") : "No company";
print(ceoCity3);

Company? company4 = null;
string ceoCity4 = "No company";
if (company4 != null) {
    ceoCity4 = company4.ceo != null ?
               (company4.ceo.address != null ?
                company4.ceo.address.city : "No address") : "No CEO";
}
print(ceoCity4);

// Test 4: Null-safe method invocation
print("Test 4: Null-safe method invocation");
Person p6 = new Person("David", new Address("Elm St", "Boston"));
string addressStr1 = p6 != null ? p6.getAddressString() : "No person";
print(addressStr1);

Person p7 = new Person("Eve", null);
string addressStr2 = p7 != null ? p7.getAddressString() : "No person";
print(addressStr2);

Person? p8 = null;
string addressStr3 = "No person";
if (p8 != null) {
    addressStr3 = p8.getAddressString();
}
print(addressStr3);

// Test 5: Null-safe navigation with function returns
function getCompany(bool returnNull): Company? {
    return returnNull ? null : new Company(new Person("ReturnedCEO", new Address("Pine St", "Seattle")));
}

print("Test 5: Null-safe navigation with function returns");
Company? funcCompany1 = getCompany(false);
string funcCeoName1 = "No company returned";
if (funcCompany1 != null) {
    funcCeoName1 = funcCompany1.getCeoName();
}
print(funcCeoName1);

Company? funcCompany2 = getCompany(true);
string funcCeoName2 = "No company returned";
if (funcCompany2 != null) {
    funcCeoName2 = funcCompany2.getCeoName();
}
print(funcCeoName2);

// Test 6: Null-safe navigation in loops
print("Test 6: Null-safe navigation in loops");
Person p9 = new Person("Loop1", new Address("First", "City1"));
Person? p10 = null;
Person p11 = new Person("Loop3", null);

for (int i = 0; i < 3; i++) {
    Person? current = null;
    if (i == 0) {
        current = p9;
    } else if (i == 1) {
        current = p10;
    } else {
        current = p11;
    }

    string result = "No person";
    if (current != null) {
        result = current.address != null ? current.address.city : "No address";
    }
    print("Iteration " + i + ": " + result);
}

// Test 7: Null-safe navigation with assignments
print("Test 7: Null-safe navigation with assignments");
Person p12 = new Person("Assignee", new Address("Maple St", "Portland"));
string extractedCity = p12 != null ? (p12.address != null ? p12.address.city : "default") : "default";
print(extractedCity);

// Test 8: Multiple null-safe navigations in one expression
print("Test 8: Multiple null-safe navigations");
Person left = new Person("Left", new Address("Left St", "LeftCity"));
Person? right = null;

string leftCity = "N/A";
if (left != null) {
    leftCity = left.address != null ? left.address.city : "N/A";
}
string rightCity = "N/A";
if (right != null) {
    rightCity = right.address != null ? right.address.city : "N/A";
}
string combined = leftCity + " and " + rightCity;
print(combined);

// Test 9: Null-safe navigation with conditional execution
print("Test 9: Null-safe navigation with conditional execution");
Person conditionalPerson = new Person("Cond", new Address("Cond St", "CondCity"));
if (conditionalPerson != null && conditionalPerson.address != null) {
    print("Has address: " + conditionalPerson.address.city);
} else {
    print("No address available");
}

Person? conditionalNull = null;
if (conditionalNull != null && conditionalNull.address != null) {
    print("Has address: " + conditionalNull.address.city);
} else {
    print("No address available");
}

// Test 10: Null-safe navigation returning complex values
print("Test 10: Null-safe navigation with defaults");
Person defaultPerson = new Person("Default", new Address("Default St", "DefaultCity"));
Person? maybePerson = null;

Person selected = maybePerson != null ? maybePerson : defaultPerson;
string finalCity = selected.address != null ? selected.address.city : "FallbackCity";
print(finalCity);

print("Test passed");
