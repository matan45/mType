// Test: A static method's declared return type is recovered at the call site
// even when the method has multiple overloads. The 2-arg overload of
// Factory::create returns Item, so consumer.accept(Factory::create(...))
// type-checks. Regression test for MYT-223.

class Item {
    public string label;
    public constructor(string l) { this.label = l; }
}

class Factory {
    public static function create(string id): Item {
        Item result = new Item("default");
        if (id == "x") {
            result = new Item("specialX");
        }
        return result;
    }

    public static function create(string id, string label): Item {
        Item result = new Item("default");
        if (id == "x") {
            result = new Item(label);
        }
        return result;
    }
}

class Consumer {
    public function accept(Item i): void {
        print("got " + i.label);
    }
}

Consumer c = new Consumer();
c.accept(Factory::create("x", "fromOverload"));
