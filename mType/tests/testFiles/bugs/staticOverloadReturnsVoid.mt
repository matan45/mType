// MYT-223: Static method overload call types as void at call site even with
// declared concrete return type.
//
// EXPECTED:
//   The call Factory::create("x", "id", "label") returns Item (declared type),
//   so consumer.accept(Factory::create(...)) type-checks. Output: "got Item"
//
// ACTUAL (broken):
//   Type error: parameter 1 expects Item but got void
//   The function body has a single return at end of a clear if/else flow —
//   yet the call site sees the expression type as void. Removing the
//   second overload fixes the issue, suggesting the bug is in overload
//   resolution / return-type recovery for static method overloads.

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
