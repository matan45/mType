// Test: @Override annotation with grandparent class method
// Expected: Pass - method overrides method from grandparent

class GrandParent {
    public function ancestorMethod(): void {
        print("GrandParent method");
    }
}

class Parent extends GrandParent {
    public function parentMethod(): void {
        print("Parent method");
    }
}

class Child extends Parent {
    @Override
    public function ancestorMethod():void {
        print("Child overriding ancestor");
    }

    @Override
    public function parentMethod(): void {
        print("Child overriding parent");
    }
}

// Test execution
Child child = new Child();
child.ancestorMethod();
child.parentMethod();
