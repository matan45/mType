// Test: @Override annotation with grandparent class method
// Expected: Pass - method overrides method from grandparent

class GrandParent {
    void ancestorMethod() {
        print("GrandParent method");
    }
}

class Parent extends GrandParent {
    void parentMethod() {
        print("Parent method");
    }
}

class Child extends Parent {
    @Override
    void ancestorMethod() {
        print("Child overriding ancestor");
    }

    @Override
    void parentMethod() {
        print("Child overriding parent");
    }
}

// Test execution
Child child = new Child();
child.ancestorMethod();
child.parentMethod();
