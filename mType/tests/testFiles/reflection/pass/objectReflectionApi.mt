// Test: Simplified reflection API using Object as universal type
// Tests Field.get/set, Method.invoke, Constructor.newInstance with Object parameters

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Constructor.mt";

// Test class with various field types and methods
class Person {
    public string name;
    public int age;

    public constructor(string name, int age) {
        this.name = name;
        this.age = age;
    }

    public function greet(): string {
        return "Hello, I'm " + this.name;
    }

    public function isAdult(): bool {
        return this.age >= 18;
    }

    public function toString(): string {
        return this.name + " (age " + this.age + ")";
    }
}

// === Test 1: Constructor.newInstance ===
print("=== Constructor.newInstance ===");
Class personClass = Class::forName("Person");
Constructor ctor = personClass.getConstructor(2);

Object[] ctorArgs = new Object[2];
ctorArgs[0] = "Alice";
ctorArgs[1] = 30;
Object person = ctor.newInstance(ctorArgs);
print("Created: " + person.toString());

// === Test 2: Field.get with Object ===
print("=== Field.get ===");
Field nameField = personClass.getField("name");
Field ageField = personClass.getField("age");

Object nameValue = nameField.get(person);
Object ageValue = ageField.get(person);
print("Name: " + nameValue);
print("Age: " + ageValue);

// === Test 3: Field.set with Object ===
print("=== Field.set ===");
nameField.set(person, "Bob");
ageField.set(person, 25);
print("Updated: " + person.toString());

// === Test 4: Method.invoke with Object ===
print("=== Method.invoke ===");
Method greetMethod = personClass.getMethod("greet", 0);
Object[] emptyArgs = new Object[0];
Object greeting = greetMethod.invoke(person, emptyArgs);
print("Greet: " + greeting);

Method isAdultMethod = personClass.getMethod("isAdult", 0);
Object adultResult = isAdultMethod.invoke(person, emptyArgs);
print("Is adult: " + adultResult);

// === Test 5: Object methods via reflection ===
print("=== Object methods via reflection ===");
Method toStringMethod = personClass.getMethod("toString", 0);
Object strResult = toStringMethod.invoke(person, emptyArgs);
print("toString: " + strResult);

// === Test 6: getSuperclass returns Object for plain class ===
print("=== Superclass ===");
Class superclass = personClass.getSuperclass();
if (superclass != null) {
    print("Superclass: " + superclass.getName());
} else {
    print("Superclass: null");
}

print("All object reflection API tests passed");
