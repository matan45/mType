// Test protected member access in subclasses

class Parent {
    private int privateField;
    protected int protectedField;
    public int publicField;

    constructor() {
        privateField = 10;
        protectedField = 20;
        publicField = 30;
    }

    protected function protectedMethod(): string {
        return "Parent protected method";
    }

    public function getPrivate(): int {
        return privateField;
    }
}

class Child extends Parent {
    constructor():super() {
    }

    public function accessProtected(): string {
        // Can access protected field from parent
        string result = "Protected field: " + super.protectedField;

        // Can call protected method from parent
        result = result + ", " + protectedMethod();

        return result;
    }

    public function modifyProtected(): void {
        super.protectedField = 99;  // Modify inherited field directly
    }

    protected function protectedMethod(): string {
        return "Child protected method";
    }
}

class GrandChild extends Child {
    constructor():super() {
    }

    public function accessFromGrandChild(): string {
        // Can access protected members through inheritance chain
        return "GrandChild accessing: " + protectedField + ", " + super.protectedMethod();
    }
}

function main(): void {
    print("Testing protected access in subclasses");

    Child child = new Child();
    print(child.accessProtected());

    child.modifyProtected();
    print("After modification: " + child.accessProtected());

    GrandChild grandChild = new GrandChild();
    grandChild.modifyProtected();  // Also modify grandChild's protected field
    print(grandChild.accessFromGrandChild());

    print("Protected access test completed");
}

main();
