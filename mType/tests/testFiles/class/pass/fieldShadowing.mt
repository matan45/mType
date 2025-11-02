// Test: Field shadowing in inheritance hierarchy
// Expected: Pass - child field shadows parent field

class Parent {
    public int value = 10;
    public string name = "Parent";

    public void display() {
        print("Parent.display(): value=" + this.value + ", name=" + this.name);
    }

    public void showValue() {
        print("Parent.showValue(): " + this.value);
    }
}

class Child extends Parent {
    // Shadow parent's fields
    public int value = 20;
    public string name = "Child";

    public void display() {
        print("Child.display(): value=" + this.value + ", name=" + this.name);
    }

    public void showBoth() {
        print("Child field: " + this.value);
        this.showValue();  // Calls parent's showValue which sees child's value
    }
}

class GrandChild extends Child {
    // Shadow child's and grandparent's fields
    public int value = 30;

    public void displayAll() {
        print("GrandChild value: " + this.value);
        print("Inherited name: " + this.name);
    }
}

// Test field shadowing
print("Test 1: Parent object");
Parent p = new Parent();
p.display();
p.showValue();

print("\nTest 2: Child object");
Child c = new Child();
c.display();
c.showBoth();

print("\nTest 3: GrandChild object");
GrandChild gc = new GrandChild();
gc.displayAll();
gc.display();

print("\nTest 4: Polymorphic reference");
Parent polyRef = new Child();
polyRef.display();  // Calls Child's display
print("Direct access: " + polyRef.value);  // Accesses based on reference type
