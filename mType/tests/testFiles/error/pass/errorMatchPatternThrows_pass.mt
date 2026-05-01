// Test: a match arm with a block body executes a throw; the
// exception propagates out of the match to a surrounding catch.
import * from "../../lib/exceptions/Exception.mt";

abstract class Shape { abstract function name(): string; }
class Circle extends Shape { public function name(): string { return "circle"; } }
class Triangle extends Shape { public function name(): string { return "triangle"; } }
class Square extends Shape { public function name(): string { return "square"; } }

function describe(Shape s): void {
    match (s) {
        case Triangle t -> {
            throw new Exception("triangles not allowed: " + t.name());
        }
        case Circle c -> { print(c.name()); }
        case Square sq -> { print(sq.name()); }
        default -> { print("?"); }
    }
}

function main(): void {
    try { describe(new Circle()); } catch (Exception e) { print("nope"); }
    try { describe(new Triangle()); } catch (Exception e) { print("caught: " + e.getMessage()); }
    try { describe(new Square()); } catch (Exception e) { print("nope"); }
}
main();
