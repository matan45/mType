// Test: Reflection API with implicit Object inheritance
// Tests getSuperclass, getMethods includes Object methods

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Constructor.mt";

// Test class
class Animal {
    public string name;

    public constructor(string name) {
        this.name = name;
    }

    public function speak(): string {
        return this.name + " speaks";
    }

    public function toString(): string {
        return "Animal:" + this.name;
    }
}

class Dog extends Animal {
    public string breed;

    public constructor(string name, string breed) : super(name) {
        this.breed = breed;
    }
}

// === Test 1: getSuperclass returns Object for class without explicit parent ===
print("=== Superclass ===");
Class animalClass = Class::forName("Animal");
Class animalSuper = animalClass.getSuperclass();
if (animalSuper != null) {
    print("Animal superclass: " + animalSuper.getName());
} else {
    print("Animal superclass: null");
}

// === Test 2: getSuperclass chain for explicit inheritance ===
Class dogClass = Class::forName("Dog");
Class dogSuper = dogClass.getSuperclass();
if (dogSuper != null) {
    print("Dog superclass: " + dogSuper.getName());
} else {
    print("Dog superclass: null");
}

// Dog -> Animal -> Object
Class dogGrandSuper = dogSuper.getSuperclass();
if (dogGrandSuper != null) {
    print("Dog grandparent: " + dogGrandSuper.getName());
} else {
    print("Dog grandparent: null");
}

// === Test 3: Object has no superclass ===
Class objectClass = dogGrandSuper.getSuperclass();
if (objectClass != null) {
    print("Object superclass: " + objectClass.getName());
} else {
    print("Object has no superclass");
}

// === Test 4: isAssignableFrom with Object ===
print("=== isAssignableFrom ===");
Class objClass = Class::forName("Object");
print("Object assignable from Animal: " + objClass.isAssignableFrom(animalClass));
print("Object assignable from Dog: " + objClass.isAssignableFrom(dogClass));

// === Test 5: Method discovery includes inherited methods ===
print("=== Methods ===");
Method speakMethod = animalClass.getMethod("speak", 0);
print("Found speak: " + speakMethod.getName());

Method toStringMethod = animalClass.getMethod("toString", 0);
print("Found toString: " + toStringMethod.getName());

// === Test 6: Field discovery and native access ===
print("=== Fields ===");
Field nameField = animalClass.getField("name");
print("Found field: " + nameField.getName());
print("Field type: " + nameField.getType());

print("All object reflection API tests passed");
