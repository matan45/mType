// Regression for MYT-301: arr[i] aliases in SoA object arrays must preserve
// normal class reference semantics.

class Box {
    public int v;

    public constructor(int v) {
        this.v = v;
    }
}

print("Testing object array element alias writes");

// >= 16 elements triggers SoA object storage.
Box[] big = new Box[20];
for (int i = 0; i < 20; i = i + 1) {
    big[i] = new Box(0);
}

big[6].v = 77;
print("Direct write before alias: " + big[6].v);

Box a = big[5];
a.v = 99;
print("Big alias write: " + big[5].v);

Box c = big[10];
Box d = big[10];
c.v = 1;
d.v = 2;
print("Multiple aliases: " + big[10].v);

big[7].v = 88;
print("Direct write after alias: " + big[7].v);

// < 16 elements stays on the normal object-array path.
Box[] small = new Box[4];
for (int i = 0; i < 4; i = i + 1) {
    small[i] = new Box(0);
}

Box b = small[2];
b.v = 77;
print("Small alias write: " + small[2].v);

print("Object array element alias write test completed");
