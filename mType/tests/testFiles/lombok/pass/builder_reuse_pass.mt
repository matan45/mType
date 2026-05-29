// Two separate builder() chains produce independent instances.
@Builder
@Getter
class Pt {
    private int x;
    private int y;
}

Pt a = Pt::builder().x(1).y(2).build();
Pt b = Pt::builder().x(3).y(4).build();
print(a.getX());
print(a.getY());
print(b.getX());
print(b.getY());
