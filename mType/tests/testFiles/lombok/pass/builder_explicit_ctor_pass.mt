// When the class already declares an all-args constructor, @Builder reuses it
// (idempotent — it must NOT synthesize a duplicate) and build() calls it.
@Builder
@Getter
class Box {
    private int v;

    public constructor(int v) {
        this.v = v;
    }
}

Box bx = Box::builder().v(42).build();
print(bx.getV());
