// Test super field access through multiple inheritance levels

class GrandParent {
    protected int level1Field;

    constructor() {
        level1Field = 100;
    }

    public function getLevel1(): int {
        return level1Field;
    }
}

class Parent extends GrandParent {
    protected int level2Field;

    constructor(): super() {
        level2Field = 200;
    }

    public function getLevel2(): int {
        return level2Field;
    }

    // Access super field from grandparent
    public function modifyGrandParentField(): void {
        super.level1Field = 150;
    }
}

class Child extends Parent {
    protected int level3Field;

    constructor(): super() {
        level3Field = 300;
    }

    public function getLevel3(): int {
        return level3Field;
    }

    // Access super field from parent
    public function modifyParentField(): void {
        super.level2Field = 250;
    }

    // Access super field from grandparent (through parent)
    public function modifyGrandParentFieldFromChild(): void {
        super.level1Field = 175;
    }

    // Read all fields via super
    public function readAllSuperFields(): string {
        return "L1: " + super.level1Field + ", L2: " + super.level2Field;
    }

    // Modify all super fields
    public function modifyAllSuperFields(): void {
        super.level1Field = 111;
        super.level2Field = 222;
        level3Field = 333;
    }
}

function main(): void {
    print("Testing super field access through inheritance chain");

    Child child = new Child();

    print("Initial values:");
    print(child.readAllSuperFields());
    print("L3: " + child.getLevel3());

    // Modify parent field
    child.modifyParentField();
    print("After modifying parent field:");
    print(child.readAllSuperFields());

    // Modify grandparent field from child
    child.modifyGrandParentFieldFromChild();
    print("After modifying grandparent field from child:");
    print(child.readAllSuperFields());

    // Modify all fields
    child.modifyAllSuperFields();
    print("After modifying all fields:");
    print(child.readAllSuperFields());
    print("L3: " + child.getLevel3());

    print("Inheritance chain test completed");
}

main();
