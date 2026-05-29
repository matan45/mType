// Field-level @Getter (the @Target permits FIELD): only the annotated fields
// receive accessors. `secret` carries no @Getter, so no getSecret() is
// generated — proving per-field opt-in, not a blanket class-level apply.
class Box {
    @Getter private int width;
    @Getter private int height;
    private int secret;

    public constructor(int w, int h, int s) {
        this.width = w;
        this.height = h;
        this.secret = s;
    }
}

Box b = new Box(3, 4, 9);
print(b.getWidth());
print(b.getHeight());
