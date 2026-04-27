// MYT-212: super.x reads child's shadowed field instead of parent's.
//
// EXPECTED:
//   this.x:                    2
//   super.x:                   1   (parent's field, by static binding)
//   child.x direct:            2
//   parent.x via parent ref:   1   (parent reference's static type wins)
//
// ACTUAL (broken):
//   All four reads return 2 (field access is dynamically dispatched).

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
