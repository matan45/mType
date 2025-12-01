// Basic reflection test
// Tests Class.forName, field introspection, and modifier checking

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Modifier.mt";

// Test class with various members
class Person {
    private string name;
    public int age;
    public static int count = 0;

    public constructor(string name, int age) {
        this.name = name;
        this.age = age;
        count = count + 1;
    }

    public function getName(): string {
        return this.name;
    }

    public function setName(string name): void {
        this.name = name;
    }

    private function secretMethod(): string {
        return "secret";
    }

    public static function getCount(): int {
        return count;
    }
}

// Test Class.forName
Class personClass = Class::forName("Person");
print("Class name: " + personClass.getName());
print("Is abstract: " + personClass.isAbstract());
print("Is final: " + personClass.isFinal());

// Test getDeclaredFields
Field[] fields = personClass.getDeclaredFields();
print("Number of declared fields: " + fields.length);

for (Field f : fields) {
    print("  Field: " + f.getName() + " type=" + f.getType() + " static=" + f.isStatic());
}

// Test getDeclaredMethods
Method[] methods = personClass.getDeclaredMethods();
print("Number of declared methods: " + methods.length);

for (Method m : methods) {
    print("  Method: " + m.getName() + " params=" + m.getParameterCount() + " static=" + m.isStatic());
}

// Test static field access
Field countField = personClass.getDeclaredField("count");
print("Static count: " + countField.getInt(0));

// Test modifier constants
int publicStaticMod = Modifier::combine(Modifier::PUBLIC, Modifier::STATIC);
print("PUBLIC + STATIC = " + publicStaticMod);
print("Is public: " + Modifier::isPublic(publicStaticMod));
print("Is static: " + Modifier::isStatic(publicStaticMod));
print("Is private: " + Modifier::isPrivate(publicStaticMod));

print("Test passed");
