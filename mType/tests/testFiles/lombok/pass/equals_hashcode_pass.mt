// @EqualsAndHashCode defers to the structural-equality optimizer pass: two
// instances with equal fields compare equal and hash identically; differing
// fields compare unequal.
@EqualsAndHashCode
@AllArgsConstructor
class Coord {
    private int a;
    private int b;
}

Coord p = new Coord(5, 6);
Coord q = new Coord(5, 6);
Coord r = new Coord(5, 7);
print(p.equals(q));
print(p.equals(r));
print(p.hashCode() == q.hashCode());
