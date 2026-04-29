// MYT-212: field access uses static binding by receiver type.
//
// `super.x` reads the parent's slot, not the most-derived shadowed slot.
// `((Parent)c).x` and `Parent p = c; p.x` likewise see the parent's value
// when the receiver's static type is Parent.

class Parent {
    public int x = 1;
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

Parent p = c;
print("parent.x via parent ref: " + p.x);
