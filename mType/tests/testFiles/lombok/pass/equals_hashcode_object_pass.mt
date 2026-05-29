// @EqualsAndHashCode over OBJECT-typed fields: equality recurses through each
// field's own equals(). Point is @Data (structural equals on its int fields),
// so two Segments built from structurally-equal Points compare equal even
// though the Point instances differ by reference.
@Data
class Point {
    private final int x;
    private final int y;
}

@EqualsAndHashCode
@AllArgsConstructor
class Segment {
    private Point start;
    private Point end;
}

Point p1 = new Point(0, 0);
Point p2 = new Point(0, 0);
Point shared = new Point(1, 1);
Segment s1 = new Segment(p1, shared);
Segment s2 = new Segment(p2, shared);
print(s1.equals(s2));
print(s1.hashCode() == s2.hashCode());
