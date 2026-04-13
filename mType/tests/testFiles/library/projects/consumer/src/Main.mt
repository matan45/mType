// Consumer: Uses MathLib library via import lib
import lib "MathLib";

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Test static method calls from library
        print("max(3,7) = " + MathUtils::max(3, 7));
        print("min(3,7) = " + MathUtils::min(3, 7));
        print("abs(-5) = " + MathUtils::abs(-5));
        print("clamp(15,0,10) = " + MathUtils::clamp(15, 0, 10));

        // Test class instantiation from library
        Vector2 a = new Vector2(1.0, 2.0);
        Vector2 b = new Vector2(3.0, 4.0);
        Vector2 c = a.add(b);
        print("Vector add: " + c.toString());

        float dot = a.dot(b);
        print("Dot product: " + dot);

        // Test inheritance from library
        Circle circle = new Circle(5.0);
        print("Circle: " + circle.describe());

        Rectangle rect = new Rectangle(4.0, 6.0);
        print("Rectangle: " + rect.describe());

        // Test polymorphism with library types
        Shape shape = circle;
        print("Polymorphic: " + shape.describe());

        print("Consumer test passed");
    }
}
