// Combo 03: Import Alias + Generics + Interface + Box Classes
// Tests: import box types with alias, implement generic interface with aliased types

import {Int as Number} from "../../lib/primitives/Int.mt";
import {String as Text} from "../../lib/primitives/String.mt";
import {Float as Decimal} from "../../lib/primitives/Float.mt";
import * from "../../lib/collections/ArrayList.mt";

interface Convertible<T, R> {
    function convert(T input): R;
}

interface Displayable {
    function display(): string;
}

// Using aliased types in class
class NumericEntry implements Displayable {
    private Number id;
    private Text label;
    private Decimal amount;

    public constructor(Number id, Text label, Decimal amount) {
        this.id = id;
        this.label = label;
        this.amount = amount;
    }

    public function getId(): Number { return this.id; }
    public function getLabel(): Text { return this.label; }
    public function getAmount(): Decimal { return this.amount; }

    public function display(): string {
        return "#" + this.id + " " + this.label + ": " + this.amount;
    }
}

// Generic converter using aliased types
class NumberToText implements Convertible<Number, Text> {
    private Text prefix;

    public constructor(Text prefix) {
        this.prefix = prefix;
    }

    public function convert(Number input): Text {
        return new Text(this.prefix.getValue() + input.getValue());
    }
}

// Generic container with aliased types
class Registry<T> {
    private ArrayList<T> items;

    public constructor() {
        this.items = new ArrayList<T>();
    }

    public function register(T item): void {
        this.items.add(item);
    }

    public function getAll(): ArrayList<T> {
        return this.items;
    }

    public function size(): int {
        return this.items.size();
    }
}

function main(): void {
    print("=== Combo 03: Import Alias + Generics + Interface ===");

    // Use aliased box types
    Number n1 = new Number(42);
    Number n2 = new Number(99);
    Text t1 = new Text("hello");
    Decimal d1 = new Decimal(3.14);

    print("Number: " + n1.getValue());
    print("Text: " + t1.getValue());
    print("Decimal: " + d1);

    // Generic interface with aliased types
    print("--- Converter ---");
    NumberToText converter = new NumberToText(new Text("Item-"));
    Text result1 = converter.convert(n1);
    Text result2 = converter.convert(n2);
    print(result1.getValue());
    print(result2.getValue());

    // Registry with aliased type entries
    print("--- Registry ---");
    Registry<NumericEntry> registry = new Registry<NumericEntry>();
    registry.register(new NumericEntry(new Number(1), new Text("Alpha"), new Decimal(10.5)));
    registry.register(new NumericEntry(new Number(2), new Text("Beta"), new Decimal(20.0)));
    registry.register(new NumericEntry(new Number(3), new Text("Gamma"), new Decimal(30.75)));

    print("Registered: " + registry.size());

    ArrayList<NumericEntry> all = registry.getAll();
    for (int i = 0; i < all.size(); i++) {
        NumericEntry entry = all.get(i);
        print(entry.display());
    }

    // Displayable interface polymorphism
    print("--- Polymorphic display ---");
    Displayable d = new NumericEntry(new Number(99), new Text("Final"), new Decimal(0.0));
    print(d.display());

    print("=== Combo 03 Complete ===");
}

main();
