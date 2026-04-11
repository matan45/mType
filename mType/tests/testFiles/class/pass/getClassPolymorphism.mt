// MYT-42: Object o = new Child(); o.getClass() returns the runtime class
// (Child), not the declared class (Animal). Polymorphism is inherited from
// the existing toString() dispatch via findInstanceMethodCached.

import * from "../../lib/reflect/Class.mt";

class Animal {
    public constructor() {}
}
class Dog extends Animal {
    public constructor() {}
}

Animal a = new Dog();
Class c = a.getClass();
print("runtime class: " + c.getName());
print("Test passed");
