// Consumer: Uses MathLib library via import lib
// Tests classes, static methods, inheritance, generics, try-catch, and reflection across library boundary
import lib "MathLib";

import * from "../../../../lib/primitives/Int.mt";
import * from "../../../../lib/primitives/String.mt";
import * from "../../../../lib/reflect/Class.mt";
import * from "../../../../lib/reflect/Field.mt";
import * from "../../../../lib/reflect/Method.mt";

@EntryPoint
class App {
    public static function main(string[] args): void {
        // === Static method calls from library ===
        print("max(3,7) = " + MathUtils::max(3, 7));
        print("min(3,7) = " + MathUtils::min(3, 7));
        print("abs(-5) = " + MathUtils::abs(-5));
        print("clamp(15,0,10) = " + MathUtils::clamp(15, 0, 10));

        // === Class instantiation from library ===
        Vector2 a = new Vector2(1.0, 2.0);
        Vector2 b = new Vector2(3.0, 4.0);
        Vector2 c = a.add(b);
        print("Vector add: " + c.toString());

        float dot = a.dot(b);
        print("Dot product: " + dot);

        // === Inheritance from library ===
        Circle circle = new Circle(5.0);
        print("Circle: " + circle.describe());

        Rectangle rect = new Rectangle(4.0, 6.0);
        print("Rectangle: " + rect.describe());

        // === Polymorphism with library types ===
        Shape shape = circle;
        print("Polymorphic: " + shape.describe());

        // === Generics from library ===
        Container<Int> intBox = new Container<Int>(new Int(42));
        print("Container<Int>: " + intBox.getValue());

        Container<String> strBox = new Container<String>("hello");
        print("Container<String>: " + strBox.getValue());

        intBox.setValue(new Int(99));
        print("Updated container: " + intBox.getValue());

        // === Try-catch with library exceptions ===
        // MathException and DivisionByZeroException come from MathLib .mtcLib
        try {
            int result = SafeMath::divide(10, 2);
            print("10 / 2 = " + result);
        } catch (MathException e) {
            print("Unexpected error: " + e.getMessage());
        }

        try {
            int bad = SafeMath::divide(10, 0);
            print("Should not reach here");
        } catch (DivisionByZeroException e) {
            print("Caught: " + e.getMessage());
        }

        try {
            float sq = SafeMath::sqrt(-1);
            print("Should not reach here");
        } catch (MathException e) {
            print("Caught: " + e.getMessage());
        }

        // === Reflection on library types ===
        Class vectorClass = Class::forName("Vector2");
        print("Class name: " + vectorClass.getName());

        Field[] fields = vectorClass.getDeclaredFields();
        print("Field count: " + fields.length);

        Method[] methods = vectorClass.getDeclaredMethods();
        print("Method count: " + methods.length);

        // === Type checking with library types ===
        Shape s = new Circle(3.0);
        print("Is Circle: " + (s isClassOf Circle));
        print("Is Shape: " + (s isClassOf Shape));

        if (s isClassOf Circle) {
            Circle castCircle = (Circle)s;
            print("Cast radius: " + castCircle.radius);
        }

        print("Consumer test passed");
    }
}
