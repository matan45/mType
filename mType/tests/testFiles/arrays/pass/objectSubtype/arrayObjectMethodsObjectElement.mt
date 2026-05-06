// MYT-282: toString/getClass on arrays of object element types
// (Animal[], etc.) — NativeArray's elementTypeName carries the class name
// for object-element arrays just as it does for primitives.
import * from "../../../lib/Object.mt";

class Animal {
    public string name;
    constructor(string n) {
        this.name = n;
    }
}

print("Testing array Object methods with object element");

Animal[] zoo = new Animal[2];
zoo[0] = new Animal("Rex");
zoo[1] = new Animal("Buddy");

Object x = zoo;
print("Animal[] toString: " + x.toString());
print("Animal[] getClass: " + x.getClass());

print("Done");
