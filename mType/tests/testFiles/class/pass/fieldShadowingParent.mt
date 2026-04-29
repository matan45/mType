// Field shadowing where child redeclares the same field name; both
// values are reachable via this.x and super.x respectively.

class Parent {
    public int x = 1;

    public function getParentX(): int {
        return this.x;
    }
}

class Child extends Parent {
    public int x = 2;

    public function getThisX(): int {
        return this.x;
    }

    public function getSuperX(): int {
        return super.x;
    }
}

Child c = new Child();

print("this.x: " + c.getThisX());
print("super.x: " + c.getSuperX());
print("child.x direct: " + c.x);

// When viewed through a Parent reference, the parent's field is visible
Parent p = c;
print("parent.x via parent ref: " + p.x);
