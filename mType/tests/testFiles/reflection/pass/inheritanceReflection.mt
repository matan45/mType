// Test reflection on inherited members

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";
import * from "../../lib/reflect/Method.mt";

class Animal {
    public string name;

    public constructor(string name) {
        this.name = name;
    }
}

class Dog extends Animal {
    public string breed;

    public constructor(string name, string breed) : super(name) {
        this.breed = breed;
    }
}

// Test Dog class reflection
Class dogClass = Class::forName("Dog");

// Get declared fields (only Dog's fields)
Field[] dogFields = dogClass.getDeclaredFields();
print("Dog declared fields: " + dogFields.length);
for (Field f : dogFields) {
    print("  Dog field: " + f.getName());
}

// Test Animal class reflection
Class animalClass = Class::forName("Animal");

Field[] animalFields = animalClass.getDeclaredFields();
print("Animal declared fields: " + animalFields.length);
for (Field f : animalFields) {
    print("  Animal field: " + f.getName());
}

print("Test passed");
