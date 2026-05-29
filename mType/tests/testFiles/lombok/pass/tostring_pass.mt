// @ToString synthesizes a structural toString over the class's fields.
@ToString
@AllArgsConstructor
class Point {
    private int x;
    private int y;
}

Point p = new Point(3, 4);
print(p.toString());
