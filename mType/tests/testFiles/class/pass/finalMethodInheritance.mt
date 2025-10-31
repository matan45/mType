// Test: Final method inheritance - Final methods inherited through multiple levels
// Expected: Pass - final methods propagate through inheritance chain

class GrandParent {
    public final function getFamily(): string {
        return "GrandParent family";
    }

    public function describe(): string {
        return "GrandParent";
    }
}

class Parent extends GrandParent {
    // Parent inherits final method from GrandParent - cannot override it
    // Parent can override non-final methods

    public function describe(): string {
        return "Parent extends " + super.describe();
    }
}

class Child extends Parent {
    // Child also cannot override the final method from GrandParent
    // Final modifier propagates through the inheritance chain

    public function describe(): string {
        return "Child extends " + super.describe();
    }

    public function test(): string {
        // Child can call the final method from GrandParent
        return this.getFamily();
    }
}

// Test inheritance chain with final methods
GrandParent gp = new GrandParent();
print(gp.getFamily());  // Should print: GrandParent family
print(gp.describe());  // Should print: GrandParent

Parent p = new Parent();
print(p.getFamily());  // Should print: GrandParent family (inherited)
print(p.describe());  // Should print: Parent extends GrandParent

Child c = new Child();
print(c.getFamily());  // Should print: GrandParent family (inherited)
print(c.describe());  // Should print: Child extends Parent extends GrandParent
print(c.test());  // Should print: GrandParent family
