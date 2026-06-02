// Test: ?. invokes an overridden method that delegates to super.make(), and
// also invokes an inherited (parent-defined) method on the derived instance,
// short-circuiting when the receiver is null (MYT-374).

class Label {
    string text;
    constructor(string t) { text = t; }
    public function getText(): string { return text; }
}

class Base {
    public function make(): Label { return new Label("Base"); }
    public function shared(): Label { return new Label("SharedFromBase"); }
}

class Derived extends Base {
    public function make(): Label {
        Label baseLabel = super.make();
        return new Label(baseLabel.getText() + "+Derived");
    }
}

function main(): void {
    Base? d = new Derived();

    // overridden method that internally calls super.make(), invoked via ?.
    Label? m = d?.make();
    if (m != null) { print(m.getText()); }

    // inherited (parent-defined) method invoked via ?. on the derived instance
    Label? s = d?.shared();
    if (s != null) { print(s.getText()); }

    Base? n = null;
    Label? gone = n?.make();
    if (gone == null) { print("short-circuited"); }
}

main();

// Expected output:
// Base+Derived
// SharedFromBase
// short-circuited
