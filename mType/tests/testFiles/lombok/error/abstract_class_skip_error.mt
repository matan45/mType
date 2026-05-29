// Abstract classes are skipped by Lombok synthesis, so @Getter on Shape never
// generates getSides(); calling it on a concrete subclass instance fails to
// resolve (the subclass declares no @Getter of its own).
@Getter
abstract class Shape {
    protected int sides;
    abstract function name(): string;
}

class Triangle extends Shape {
    public function name(): string {
        return "triangle";
    }
}

Triangle t = new Triangle();
print(t.getSides());
