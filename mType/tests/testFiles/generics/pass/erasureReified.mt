import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Reified generics simulation via explicit type passing
class Factory<T> {
    String className;

    public function Factory(String name) {
        className = name;
    }

    public function getClassName(): String {
        return className;
    }
}

class Product {
    String name;

    public function Product(String n) {
        name = n;
    }

    public function getName(): String {
        return name;
    }
}

class Widget extends Product {
    public function Widget(String n) {
        super(n);
    }
}

function main(): void {
    // Simulate reified generics by passing type info
    Factory<Widget> widgetFactory = new Factory<Widget>(new String("Widget"));
    print("Creating: " + widgetFactory.getClassName());

    Factory<Product> productFactory = new Factory<Product>(new String("Product"));
    print("Creating: " + productFactory.getClassName());
}

main();
