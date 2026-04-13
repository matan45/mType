value class Color {
    public int r;
    public int g;
    public int b;

    public constructor(int r, int g, int b) {
        this.r = r;
        this.g = g;
        this.b = b;
    }
}

Color c1 = new Color(255, 0, 0);
Color c2 = c1;
print(c1.r);
print(c2.r);
