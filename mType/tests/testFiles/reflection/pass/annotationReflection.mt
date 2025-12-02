// Test annotation reflection

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

// Base class with a method to override
class BaseClass {
    public function doSomething(): void {
        print("base");
    }
}

// Class with @Override annotation on method
class DerivedClass extends BaseClass {
    @Override
    public function doSomething(): void {
        print("derived");
    }
}

class NonAnnotatedClass {
    public int x;
}

// Test class with method that has annotation
Class derivedClass = Class::forName("DerivedClass");
print("DerivedClass annotation count: " + derivedClass.getAnnotations().length);

// Test non-annotated class
Class normalClass = Class::forName("NonAnnotatedClass");
print("NonAnnotatedClass annotation count: " + normalClass.getAnnotations().length);

print("Test passed");
