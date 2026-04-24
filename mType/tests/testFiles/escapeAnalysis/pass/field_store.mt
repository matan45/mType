class Inner {
    public int v;
    public constructor(int a) {
        this.v = a;
    }
}

class Box {
    public Inner held;
    public constructor() {
    }
}

Box b = new Box();
Inner x = new Inner(13);
b.held = x;
print(b.held.v);
