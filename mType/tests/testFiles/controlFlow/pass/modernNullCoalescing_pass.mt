// Modern null coalescing patterns using ternary operators
// Tests null-safe value selection and default value assignment
import { String } from "../../lib/primitives/String.mt";

class Person {
    public string name;
    public int age;

    constructor(string n, int a) {
        name = n;
        age = a;
    }

    public function toString(): string {
        return name + " (" + age + ")";
    }
}

// Test 1: Simple null coalescing with objects
print("Test 1: Simple null coalescing");
Person? p1 = null;
Person defaultPerson = new Person("Default", 0);
Person result1 = p1 != null ? p1 : defaultPerson;
print(result1.toString());

// Test 2: Null coalescing with valid object
Person p2 = new Person("Alice", 30);
Person result2 = p2 != null ? p2 : defaultPerson;
print(result2.toString());

// Test 3: Chained null coalescing (multiple fallbacks)
print("Test 2: Chained null coalescing");
Person? primary = null;
Person? secondary = null;
Person tertiary = new Person("Tertiary", 25);
Person fallback = new Person("Fallback", 99);
Person result3 = primary != null ? primary : (secondary != null ? secondary : (tertiary != null ? tertiary : fallback));
print(result3.toString());

// Test 4: Null coalescing with function returns
function getPerson(bool shouldReturnNull): Person? {
    return shouldReturnNull ? null : new Person("Function Person", 45);
}

print("Test 3: Null coalescing with function returns");
Person result4 = getPerson(true) != null ? getPerson(true) : new Person("Default Function", 50);
print(result4.toString());

Person result5 = getPerson(false) != null ? getPerson(false) : new Person("Default Function", 50);
print(result5.toString());

// Test 5: Null coalescing with string values
print("Test 4: String null coalescing");
String? s1 = null;
string defaultString = "default value";
string strResult1 = defaultString;
if (s1 != null) {
    strResult1 = s1.toString();
}
print(strResult1);

String s2 = new String("actual value");
string strResult2 = s2 != null ? s2.toString() : defaultString;
print(strResult2);

// Test 6: Null coalescing in assignments
print("Test 5: Null coalescing in variable assignment");
Person? temp = null;
Person assigned = temp != null ? temp : new Person("Assigned", 33);
print(assigned.toString());

// Test 7: Nested null coalescing with method calls
class Container {
    public Person? person;

    constructor(Person? p) {
        person = p;
    }

    public function getPerson(): Person? {
        return person;
    }
}

print("Test 6: Nested null coalescing");
Container? c1 = null;
Person? noOne = null;
Container c2 = new Container(noOne);
Container c3 = new Container(new Person("Nested", 28));
Person safeDefault = new Person("Safe", 100);

Person result6 = safeDefault;
if (c1 != null) {
    Person? cp1 = c1.getPerson();
    result6 = cp1 != null ? cp1 : safeDefault;
}
print(result6.toString());

Person result7 = c2 != null ? (c2.getPerson() != null ? c2.getPerson() : safeDefault) : safeDefault;
print(result7.toString());

Person result8 = c3 != null ? (c3.getPerson() != null ? c3.getPerson() : safeDefault) : safeDefault;
print(result8.toString());

// Test 8: Null coalescing with integer values through objects
class Counter {
    public int value;

    constructor(int v) {
        value = v;
    }
}

print("Test 7: Null coalescing with wrapped values");
Counter? cnt1 = null;
Counter cnt2 = new Counter(42);
int defaultValue = -1;

int countResult1 = defaultValue;
if (cnt1 != null) {
    countResult1 = cnt1.value;
}
print(countResult1);

int countResult2 = cnt2 != null ? cnt2.value : defaultValue;
print(countResult2);

// Test 9: Null coalescing in conditional statements
print("Test 8: Null coalescing in conditionals");
Person? checkPerson = null;
if ((checkPerson != null ? checkPerson : defaultPerson).age > 0) {
    print("Person exists with positive age");
}

// Test 10: Multiple null coalescing in single expression
print("Test 9: Multiple null coalescing operations");
Person? first = null;
Person second = new Person("Second", 20);
Person third = new Person("Third", 30);

string firstName = "null";
if (first != null) {
    firstName = first.name;
}
string combined = firstName + " | " +
                  (second != null ? second.name : "null") + " | " +
                  (third != null ? third.name : "null");
print(combined);

print("Test passed");
