// Integration Test 20: Decorator Pattern with Inheritance
// Tests: Inheritance chains + Override + Super calls + Composition

import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Float.mt";

// Component interface
interface Coffee {
    function getCost(): float;
    function getDescription(): string;
}

// Base component
class SimpleCoffee implements Coffee {
    public function getCost(): float {
        return 2.0;
    }

    public function getDescription(): string {
        return "Simple Coffee";
    }
}

// Abstract decorator
abstract class CoffeeDecorator implements Coffee {
    protected Coffee decoratedCoffee;

    constructor(Coffee c) {
        this.decoratedCoffee = c;
    }

    public function getCost(): float {
        return this.decoratedCoffee.getCost();
    }

    public function getDescription(): string {
        return this.decoratedCoffee.getDescription();
    }
}

// Concrete decorators
class MilkDecorator extends CoffeeDecorator {
    constructor(Coffee c) : super(c) {}

    @Override
    public function getCost(): float {
        return super.getCost() + 0.5;
    }

    @Override
    public function getDescription(): string {
        return super.getDescription() + ", Milk";
    }
}

class SugarDecorator extends CoffeeDecorator {
    private int cubes;

    constructor(Coffee c, int cubes) : super(c) {
        this.cubes = cubes;
    }

    @Override
    public function getCost(): float {
        return super.getCost() + (0.2 * this.cubes);
    }

    @Override
    public function getDescription(): string {
        return super.getDescription() + ", Sugar(" + this.cubes + ")";
    }
}

class WhipCreamDecorator extends CoffeeDecorator {
    constructor(Coffee c) : super(c) {}

    @Override
    public function getCost(): float {
        return super.getCost() + 0.7;
    }

    @Override
    public function getDescription(): string {
        return super.getDescription() + ", Whip Cream";
    }
}

class CaramelDecorator extends CoffeeDecorator {
    constructor(Coffee c) : super(c) {}

    @Override
    public function getCost(): float {
        return super.getCost() + 0.6;
    }

    @Override
    public function getDescription(): string {
        return super.getDescription() + ", Caramel";
    }
}

// Text formatting decorator pattern
interface TextFormatter {
    function format(string text): string;
}

class PlainTextFormatter implements TextFormatter {
    public function format(string text): string {
        return text;
    }
}

class BoldDecorator implements TextFormatter {
    private TextFormatter formatter;

    constructor(TextFormatter f) {
        this.formatter = f;
    }

    public function format(string text): string {
        return "<b>" + this.formatter.format(text) + "</b>";
    }
}

class ItalicDecorator implements TextFormatter {
    private TextFormatter formatter;

    constructor(TextFormatter f) {
        this.formatter = f;
    }

    public function format(string text): string {
        return "<i>" + this.formatter.format(text) + "</i>";
    }
}

class UnderlineDecorator implements TextFormatter {
    private TextFormatter formatter;

    constructor(TextFormatter f) {
        this.formatter = f;
    }

    public function format(string text): string {
        return "<u>" + this.formatter.format(text) + "</u>";
    }
}

// Helper function to display coffee
function displayCoffee(Coffee c): void {
    print("Order: " + c.getDescription());
    print("Cost: $" + c.getCost());
}

// Main test execution
print("=== Test 20: Decorator Pattern with Inheritance ===");

// Test 1: Simple coffee
print("--- Test 1: Simple coffee ---");
Coffee simple = new SimpleCoffee();
displayCoffee(simple);

// Test 2: Coffee with milk
print("--- Test 2: Coffee with milk ---");
Coffee withMilk = new MilkDecorator(new SimpleCoffee());
displayCoffee(withMilk);

// Test 3: Coffee with milk and sugar
print("--- Test 3: Coffee with milk and sugar ---");
Coffee withMilkAndSugar = new SugarDecorator(
    new MilkDecorator(new SimpleCoffee()),
    2
);
displayCoffee(withMilkAndSugar);

// Test 4: Fully loaded coffee
print("--- Test 4: Fully loaded coffee ---");
Coffee fullyLoaded = new CaramelDecorator(
    new WhipCreamDecorator(
        new SugarDecorator(
            new MilkDecorator(new SimpleCoffee()),
            3
        )
    )
);
displayCoffee(fullyLoaded);

// Test 5: Multiple decorations in different order
print("--- Test 5: Different order ---");
Coffee order1 = new WhipCreamDecorator(
    new MilkDecorator(new SimpleCoffee())
);
displayCoffee(order1);

Coffee order2 = new MilkDecorator(
    new WhipCreamDecorator(new SimpleCoffee())
);
displayCoffee(order2);

// Test 6: Text formatter decorators
print("--- Test 6: Text formatting ---");
TextFormatter plain = new PlainTextFormatter();
print("Plain: " + plain.format("Hello"));

TextFormatter bold = new BoldDecorator(new PlainTextFormatter());
print("Bold: " + bold.format("Hello"));

TextFormatter boldItalic = new ItalicDecorator(
    new BoldDecorator(new PlainTextFormatter())
);
print("Bold+Italic: " + boldItalic.format("Hello"));

TextFormatter all = new UnderlineDecorator(
    new ItalicDecorator(
        new BoldDecorator(new PlainTextFormatter())
    )
);
print("All: " + all.format("Hello"));

// Test 7: Multiple instances
print("--- Test 7: Multiple orders ---");
Coffee order3 = new MilkDecorator(new SimpleCoffee());
Coffee order4 = new SugarDecorator(new SimpleCoffee(), 1);
Coffee order5 = new CaramelDecorator(new SimpleCoffee());

print("Order 3: " + order3.getDescription() + " = $" + order3.getCost());
print("Order 4: " + order4.getDescription() + " = $" + order4.getCost());
print("Order 5: " + order5.getDescription() + " = $" + order5.getCost());

float total = order3.getCost() + order4.getCost() + order5.getCost();
print("Total: $" + total);

// Test 8: Deep decoration chain
print("--- Test 8: Deep decoration ---");
Coffee deep = new SugarDecorator(
    new MilkDecorator(
        new SugarDecorator(
            new WhipCreamDecorator(
                new MilkDecorator(
                    new SimpleCoffee()
                )
            ),
            2
        )
    ),
    1
);
displayCoffee(deep);

// Test 9: Type checking
print("--- Test 9: Type checking ---");
Coffee decorated = new MilkDecorator(new SimpleCoffee());

print("Is Coffee: " + (decorated isClassOf Coffee));
print("Is MilkDecorator: " + (decorated isClassOf MilkDecorator));
print("Is CoffeeDecorator: " + (decorated isClassOf CoffeeDecorator));

print("=== Test 20 Complete ===");
