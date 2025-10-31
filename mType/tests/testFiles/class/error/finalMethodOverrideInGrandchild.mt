// ERROR: Override final method in grandchild class

class GrandParent {
    public final function validate(): bool {
        return true;
    }
}

class Parent extends GrandParent {
    // Parent correctly does not override the final method
    public function otherMethod(): string {
        return "Parent method";
    }
}

class GrandChild extends Parent {
    // ERROR: Cannot override final method from GrandParent
    // Even though immediate parent (Parent) didn't override it,
    // the method is still final from GrandParent
    public function validate(): bool {
        return false;
    }
}

function main(): void {
    GrandChild gc = new GrandChild();
    print("This should not execute");
}

main();
