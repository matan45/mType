// Test superclass reflection

import * from "../../lib/reflect/Class.mt";

class GrandParent {
    public int a;
}

class Parent extends GrandParent {
    public int b;
}

class Child extends Parent {
    public int c;
}

// Test Child's superclass
Class childClass = Class::forName("Child");
print("Child class name: " + childClass.getName());

Class parentClass = childClass.getSuperclass();
if (parentClass != null) {
    print("Child's superclass: " + parentClass.getName());
} else {
    print("Child has no superclass");
}

// Test Parent's superclass
Class grandParentClass = parentClass.getSuperclass();
if (grandParentClass != null) {
    print("Parent's superclass: " + grandParentClass.getName());
} else {
    print("Parent has no superclass");
}

// Test GrandParent's superclass (should be null or Object)
Class gpSuperclass = grandParentClass.getSuperclass();
if (gpSuperclass != null) {
    print("GrandParent's superclass: " + gpSuperclass.getName());
} else {
    print("GrandParent has no superclass");
}

print("Test passed");
