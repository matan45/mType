// @Setter skips final fields (the documented contrast to @Getter, which does
// not). No setX() is generated for the final field, so the call fails to
// resolve even though @AllArgsConstructor still takes x.
@Setter
@AllArgsConstructor
class Frozen {
    private final int x;
}

Frozen f = new Frozen(5);
f.setX(10);
