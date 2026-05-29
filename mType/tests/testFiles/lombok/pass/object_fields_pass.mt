// Getters over object-typed fields return the class type; @ToString renders an
// object field via its own toString(). Exercises nested synthesized members.
@AllArgsConstructor
@Getter
@ToString
class Inner {
    private int v;
}

@AllArgsConstructor
@Getter
@ToString
class Outer {
    private Inner inner;
    private string label;
}

Inner i = new Inner(7);
Outer o = new Outer(i, "tag");
print(o.getInner().getV());
print(o.getLabel());
print(o.toString());
