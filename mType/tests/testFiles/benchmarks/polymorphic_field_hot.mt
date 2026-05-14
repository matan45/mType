// TARGET: shape coverage for polymorphic field reads.
// Four subclasses share an inherited field slot, so this isolates receiver
// shape variation from method-dispatch overhead.

class FieldBox {
    public int n;

    public constructor(int n) {
        this.n = n;
    }
}

class FieldA extends FieldBox {
    public constructor(int n) : super(n) {}
}

class FieldB extends FieldBox {
    public constructor(int n) : super(n) {}
}

class FieldC extends FieldBox {
    public constructor(int n) : super(n) {}
}

class FieldD extends FieldBox {
    public constructor(int n) : super(n) {}
}

FieldBox[] boxes = [new FieldA(3), new FieldB(5), new FieldC(7), new FieldD(11)];

int N = 2000000;
int sum = 0;
for (int i = 0; i < N; i = i + 1) {
    sum = sum + boxes[i % 4].n;
}

print("polymorphic_field_hot sum=" + sum);
