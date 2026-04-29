// Test: An overloaded static call's result may be member-accessed at the same
// expression. inferExpressionClassName must recover the return class for both
// overloads; otherwise the .label access cannot be type-checked.

class Item {
    public string label;
    public constructor(string l) { this.label = l; }
}

class Factory {
    public static function make(string id): Item {
        return new Item("one:" + id);
    }
    public static function make(string id, string suffix): Item {
        return new Item("two:" + id + suffix);
    }
}

print(Factory::make("a").label);
print(Factory::make("a", "-z").label);
